#!/usr/bin/env python3
"""
SpaceTrack data ingestion script.
Fetches satellite data from the SpaceTrack API and writes to PostgreSQL database.
"""

import logging
import os
import sys
from datetime import date

import requests
from dotenv import load_dotenv
import psycopg2
from psycopg2.extras import execute_values

from healthcheck import HealthCheck

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)

# Load environment variables
load_dotenv()

# Field mapping: JSON field -> database column
FIELD_MAPPING = {
    "OBJECT_ID": "id",
    "OBJECT_NAME": "name",
    "EPOCH": "epoch",
    "MEAN_MOTION": "mean_motion",
    "ECCENTRICITY": "eccentricity",
    "INCLINATION": "inclination",
    "RA_OF_ASC_NODE": "raan",
    "ARG_OF_PERICENTER": "arg_of_pericenter",
    "MEAN_ANOMALY": "mean_anomaly",
    "NORAD_CAT_ID": "norad_id",
    "REV_AT_EPOCH": "revolutions_at_epoch",
    "BSTAR": "bstar",
    "MEAN_MOTION_DOT": "mean_motion_dot",
    "MEAN_MOTION_DDOT": "mean_motion_ddot",
    "SEMIMAJOR_AXIS": "semimajor_axis",
    "PERIOD": "period",
    "OBJECT_TYPE": "object_type",
    "RCS_SIZE": "rcs_size",
    "COUNTRY_CODE": "country_code",
    "LAUNCH_DATE": "launch_date",
    "SITE": "launch_site",
    "CREATION_DATE": "creation_date",
}

# Database columns in order for insert
DB_COLUMNS = list(FIELD_MAPPING.values())

BATCH_SIZE = 1000

GP_DATA_URL = (
    "https://www.space-track.org/basicspacedata/query"
    "/class/gp/EPOCH/>now-30/orderby/NORAD_CAT_ID,EPOCH/format/json"
)
ANALYST_DATA_URL = (
    "https://www.space-track.org/basicspacedata/query"
    "/class/gp/EPOCH/%3Enow-30/NORAD_CAT_ID/80000--89999"
    "/orderby/NORAD_CAT_ID/format/json/emptyresult/show"
)

HEALTHCHECK_ENDPOINT = "https://hc-ping.com/3d1ac17f-dd1f-46c5-b12d-4570bb56c5de"

def connect_db():
    """Establish PostgreSQL connection using environment variables."""
    try:
        conn = psycopg2.connect(
            host=os.getenv("DB_HOST", "localhost"),
            port=os.getenv("DB_PORT", "5432"),
            dbname=os.getenv("DB_NAME", "orbis"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD", ""),
        )
        logger.info("Connected to database")
        return conn
    except psycopg2.Error as e:
        logger.error(f"Failed to connect to database: {e}")
        sys.exit(1)


def fetch_data():
    """Fetch GP and analyst satellite data from the SpaceTrack API."""
    user = os.getenv("SPACETRACK_USER")
    password = os.getenv("SPACETRACK_PASSWORD")
    if not user or not password:
        logger.error("SPACETRACK_USER and SPACETRACK_PASSWORD must be set in .env")
        sys.exit(1)

    login_url = "https://www.space-track.org/ajaxauth/login"

    session = requests.Session()

    logger.info("Logging in to SpaceTrack...")
    resp = session.post(login_url, data={"identity": user, "password": password})
    if resp.status_code != 200:
        logger.error(f"SpaceTrack login failed (HTTP {resp.status_code})")
        sys.exit(1)
    logger.info("SpaceTrack login successful")

    logger.info("Fetching GP data...")
    resp = session.get(GP_DATA_URL)
    if resp.status_code != 200:
        logger.error(f"GP data request failed (HTTP {resp.status_code})")
        sys.exit(1)
    gp_records = resp.json()
    logger.info(f"Fetched {len(gp_records)} GP records")

    logger.info("Fetching analyst data...")
    resp = session.get(ANALYST_DATA_URL)
    if resp.status_code != 200:
        logger.error(f"Analyst data request failed (HTTP {resp.status_code})")
        sys.exit(1)
    analyst_records = resp.json()
    logger.info(f"Fetched {len(analyst_records)} analyst records")

    return gp_records, analyst_records


def transform_record(record):
    """Transform a JSON record to database row tuple."""
    row = []
    for json_field, db_column in FIELD_MAPPING.items():
        value = record.get(json_field)
        # Handle empty strings as NULL
        if value == "":
            value = None
        row.append(value)
    return tuple(row)


def deduplicate_batch(records):
    """Remove duplicates from batch, keeping the last occurrence (most recent data)."""
    seen = {}
    norad_id_idx = DB_COLUMNS.index("norad_id")
    for record in records:
        norad_id = record[norad_id_idx]
        seen[norad_id] = record
    return list(seen.values())


def insert_batch(conn, records):
    """Batch insert records using execute_values with upsert."""
    if not records:
        return 0

    # Deduplicate within batch to avoid "cannot affect row a second time" error
    records = deduplicate_batch(records)

    columns = ", ".join(DB_COLUMNS)
    # Build the SET clause for ON CONFLICT UPDATE (exclude 'norad_id' which is the conflict key)
    update_columns = [col for col in DB_COLUMNS if col != "norad_id"]
    update_set = ", ".join([f"{col} = EXCLUDED.{col}" for col in update_columns])

    query = f"""
        INSERT INTO public.objects ({columns})
        VALUES %s
        ON CONFLICT (norad_id) DO UPDATE SET {update_set}
    """

    with conn.cursor() as cur:
        execute_values(cur, query, records, page_size=BATCH_SIZE)
    conn.commit()
    return len(records)


def insert_groups_batch(conn, ids, group):
    """Insert group memberships into public.groups."""
    if not ids:
        return 0

    unique_ids = list(set(ids))
    today = date.today()
    records = [(norad_id, group, today) for norad_id in unique_ids]

    query = """
        INSERT INTO public.groups (norad_id, "group", creation_date)
        VALUES %s
        ON CONFLICT (norad_id, "group") DO UPDATE SET creation_date = EXCLUDED.creation_date
    """

    with conn.cursor() as cur:
        execute_values(cur, query, records, page_size=BATCH_SIZE)
    conn.commit()
    logger.info(f"Inserted {len(records)} '{group}' group memberships")
    return len(records)


def clear_analyst_objects(conn):
    """Remove all analyst objects from public.objects and public.groups."""
    with conn.cursor() as cur:
        cur.execute("DELETE FROM public.groups WHERE \"group\" = 'analyst'")
        cur.execute("DELETE FROM public.objects WHERE norad_id::int BETWEEN 80000 AND 89999")
    conn.commit()
    logger.info("Cleared analyst objects")


def clear_stale_objects(conn):
    """Delete objects where creation_date is more than 3 days in the past."""
    with conn.cursor() as cur:
        cur.execute(
            "DELETE FROM public.objects WHERE creation_date < NOW() - INTERVAL '3 days'"
        )
        deleted = cur.rowcount
    conn.commit()
    logger.info(f"Cleared {deleted} stale objects")


def process_records(conn, data, label):
    """Transform and insert records in batches."""
    total = len(data)
    processed = 0
    batch = []

    for record in data:
        row = transform_record(record)
        batch.append(row)

        if len(batch) >= BATCH_SIZE:
            insert_batch(conn, batch)
            processed += len(batch)
            logger.info(f"{label}: Processed {processed}/{total} records")
            batch = []

    if batch:
        insert_batch(conn, batch)
        processed += len(batch)
        logger.info(f"{label}: Processed {processed}/{total} records")

    return processed


def main():
    """Main ingestion process."""
    hc = HealthCheck(HEALTHCHECK_ENDPOINT)
    hc.start()

    gp_data, analyst_data = fetch_data()

    conn = connect_db()

    try:
        """
        Well-tracked analyst objects are rather volatile - they don't get updated every time, and their
        creation_date can be several days in the past. So we remove them from the table with every ingestion,
        then trim the stale objects (objects with creation dates older than 3 days), and finally add all the
        analyst objects again.  
        """
        clear_analyst_objects(conn)
        clear_stale_objects(conn)

        gp_count = process_records(conn, gp_data, "GP")
        analyst_count = process_records(conn, analyst_data, "Analyst")

        analyst_ids = [record.get("NORAD_CAT_ID") for record in analyst_data if record.get("NORAD_CAT_ID")]
        insert_groups_batch(conn, analyst_ids, "analyst")

        logger.info(f"Ingestion complete. GP: {gp_count}, Analyst: {analyst_count}")
        hc.success()

    except psycopg2.Error as e:
        logger.error(f"Database error: {e}")
        conn.rollback()
        hc.fail()
        sys.exit(1)
    finally:
        conn.close()
        logger.info("Database connection closed")


if __name__ == "__main__":
    main()
