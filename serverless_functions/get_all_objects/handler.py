"""
Scaleway serverless function to retrieve all space objects and their groups from the database.
"""

import json
import os

import pg8000


def handle(event, context):
    conn = None
    try:
        database_connection = pg8000.connect(
            host=os.getenv("DB_HOST", "localhost"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "orbis"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD", ""),
        )

        response = {
            "objects": _get_objects(database_connection),
            "groups": _get_groups(database_connection)
        }

        return {
            "statusCode": 200,
            "headers": {"Content-Type": ["application/json"]},
            "body": json.dumps(response),
        }

    except pg8000.Error as e:
        return {
            "statusCode": 500,
            "headers": {"Content-Type": ["application/json"]},
            "body": json.dumps({"error": f"Database error: {str(e)}"}),
        }

    finally:
        if database_connection:
            database_connection.close()

def _get_objects(database_connection):
    rows = database_connection.run("""
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
    for row in rows:
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
    return objects

def _get_groups(database_connection):
    groups = _get_explicit_groups(database_connection) | _get_debris_group(database_connection)
    return groups

def _get_explicit_groups(database_connection):
    # "group" is in quotes in the query as it is a reserved keyword in PostgreSQL.
    rows = database_connection.run("""
        SELECT
            norad_id,
            "group"
        FROM public.groups
    """)

    groups = {}
    for row in rows:
        norad_id = row[0]
        group_name = row[1]
        if group_name not in groups:
            groups[group_name] = []
        groups[group_name].append(norad_id)
    return groups

def _get_debris_group(database_connection):
    rows = database_connection.run("""
        SELECT norad_id
        FROM public.objects
        WHERE object_type = 'DEBRIS'
    """)

    groups = {"debris": [row[0] for row in rows]}
    return groups
