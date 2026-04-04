#!/usr/bin/env python3
"""
CelesTrak data ingestion script.
Fetches satellite group data from CelesTrak and inserts NORAD IDs
with group names into the PostgreSQL database.
"""

import logging
import os
import sys
import time
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

BATCH_SIZE = 1000

CELESTRAK_URL = "https://celestrak.org/NORAD/elements/gp.php?GROUP={group}&FORMAT=json"

GROUPS = [
    "stations",
    "starlink",
    "oneweb",
    "gps-ops",
    "gnss",
    "geo",
    "science",
    "fengyun-1c-debris",
]

HEALTHCHECK_ENDPOINT = "https://hc-ping.com/06649829-a983-4e3d-b0ea-1b7257e6fd1f"


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


def fetch_group(group):
    """Fetch satellite group data from CelesTrak."""
    url = CELESTRAK_URL.format(group=group)
    logger.info(f"Fetching group '{group}' from CelesTrak...")
    resp = requests.get(url)
    if resp.status_code != 200:
        logger.error(f"CelesTrak request failed (HTTP {resp.status_code})")
        sys.exit(1)
    records = resp.json()
    logger.info(f"Fetched {len(records)} records")
    return records


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


def clear_stale_groups(conn):
    """Delete group memberships where creation_date is more than 3 days in the past."""
    with conn.cursor() as cur:
        cur.execute(
            "DELETE FROM public.groups WHERE creation_date < NOW() - INTERVAL '3 days'"
        )
        deleted = cur.rowcount
    conn.commit()
    logger.info(f"Cleared {deleted} stale group memberships")


def main():
    """Main ingestion process."""
    hc = HealthCheck(HEALTHCHECK_ENDPOINT)
    hc.start()

    conn = connect_db()
    try:
        clear_stale_groups(conn)

        for i, group in enumerate(GROUPS):
            if i > 0:
                time.sleep(5)

            records = fetch_group(group)
            norad_ids = [
                record.get("NORAD_CAT_ID")
                for record in records
                if record.get("NORAD_CAT_ID")
            ]
            insert_groups_batch(conn, norad_ids, group)

        logger.info("Ingestion complete")
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
