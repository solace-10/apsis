#!/usr/bin/env python3
"""
SpaceTrack data ingestion script.
Ingests satellite data from SpaceTrack JSON and writes to PostgreSQL database.
"""

import json
import logging
import os
import sys
from pathlib import Path

from dotenv import load_dotenv
import psycopg2
from psycopg2.extras import execute_values

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
    "REF_FRAME": "reference_frame",
    "TIME_SYSTEM": "time_system",
    "MEAN_ELEMENT_THEORY": "mean_element_theory",
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
}

# Database columns in order for insert
DB_COLUMNS = list(FIELD_MAPPING.values())

BATCH_SIZE = 1000


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


def load_json(filepath):
    """Load JSON data from file."""
    logger.info(f"Loading JSON from {filepath}")
    try:
        with open(filepath, "r") as f:
            data = json.load(f)
        logger.info(f"Loaded {len(data)} records from JSON")
        return data
    except (json.JSONDecodeError, IOError) as e:
        logger.error(f"Failed to load JSON: {e}")
        sys.exit(1)


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
    for record in records:
        # First element is the 'id' field
        object_id = record[0]
        seen[object_id] = record
    return list(seen.values())


def insert_batch(conn, records):
    """Batch insert records using execute_values with upsert."""
    if not records:
        return 0

    # Deduplicate within batch to avoid "cannot affect row a second time" error
    records = deduplicate_batch(records)

    columns = ", ".join(DB_COLUMNS)
    # Build the SET clause for ON CONFLICT UPDATE (exclude 'id' which is the conflict key)
    update_columns = [col for col in DB_COLUMNS if col != "id"]
    update_set = ", ".join([f"{col} = EXCLUDED.{col}" for col in update_columns])

    query = f"""
        INSERT INTO public.objects ({columns})
        VALUES %s
        ON CONFLICT (id) DO UPDATE SET {update_set}
    """

    with conn.cursor() as cur:
        execute_values(cur, query, records, page_size=BATCH_SIZE)
    conn.commit()
    return len(records)


def main():
    """Main ingestion process."""
    # Determine JSON file path
    script_dir = Path(__file__).parent
    json_file = script_dir / "all.json"

    if not json_file.exists():
        logger.error(f"JSON file not found: {json_file}")
        sys.exit(1)

    # Load JSON data
    data = load_json(json_file)
    total_records = len(data)

    # Connect to database
    conn = connect_db()

    try:
        # Process records in batches
        processed = 0
        batch = []

        for record in data:
            row = transform_record(record)
            batch.append(row)

            if len(batch) >= BATCH_SIZE:
                insert_batch(conn, batch)
                processed += len(batch)
                logger.info(f"Processed {processed}/{total_records} records")
                batch = []

        # Insert remaining records
        if batch:
            insert_batch(conn, batch)
            processed += len(batch)
            logger.info(f"Processed {processed}/{total_records} records")

        logger.info(f"Ingestion complete. Total records processed: {processed}")

    except psycopg2.Error as e:
        logger.error(f"Database error: {e}")
        conn.rollback()
        sys.exit(1)
    finally:
        conn.close()
        logger.info("Database connection closed")


if __name__ == "__main__":
    main()
