"""
Scaleway serverless function to retrieve all space objects from the database.
Returns NORAD ID, name, and the 6 classical Keplerian orbital elements.
"""

import json
import os

import pg8000


def handle(event, context):
    """
    Scaleway serverless function handler.
    Returns all space objects with their orbital elements.
    """
    conn = None
    try:
        conn = pg8000.connect(
            host=os.getenv("DB_HOST", "localhost"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "orbis"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD", ""),
        )

        cur = conn.run("""
            SELECT
                norad_id,
                name,
                epoch,
                mean_motion,
                eccentricity,
                inclination,
                raan,
                arg_of_pericenter,
                mean_anomaly
            FROM public.objects
        """)

        objects = []
        for row in cur:
            epoch = row[2]
            if epoch and hasattr(epoch, 'isoformat'):
                epoch = epoch.isoformat()
            objects.append({
                "norad_id": row[0],
                "name": row[1],
                "epoch": epoch,
                "mean_motion": float(row[3]) if row[3] else None,
                "eccentricity": float(row[4]) if row[4] else None,
                "inclination": float(row[5]) if row[5] else None,
                "raan": float(row[6]) if row[6] else None,
                "arg_of_pericenter": float(row[7]) if row[7] else None,
                "mean_anomaly": float(row[8]) if row[8] else None,
            })

        return {
            "statusCode": 200,
            "headers": {"Content-Type": ["application/json"]},
            "body": json.dumps(objects),
        }

    except pg8000.Error as e:
        return {
            "statusCode": 500,
            "headers": {"Content-Type": ["application/json"]},
            "body": json.dumps({"error": f"Database error: {str(e)}"}),
        }

    finally:
        if conn:
            conn.close()
