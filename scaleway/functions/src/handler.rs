use axum::{body::Body, extract::Request, response::Response};
use chrono::{NaiveDate, NaiveDateTime};
use http::{Method, StatusCode};
use serde::Serialize;
use sqlx::Connection;
use std::collections::HashMap;

type GroupCollection = HashMap<String, Vec<i32>>;

struct DatabaseConfig {
    host: String,
    port: String,
    name: String,
    user: String,
    password: String,
}

#[derive(Serialize)]
struct AllObjectsResponse {
    objects: Vec<ObjectEntry>,
    groups: GroupCollection,
}

#[derive(Serialize, sqlx::FromRow)]
struct ObjectEntry {
    id: String,
    norad_id: i32,
    name: String,
    epoch: NaiveDateTime,
    mean_motion: f64,
    eccentricity: f64,
    inclination: f64,
    raan: f64,
    arg_of_pericenter: f64,
    mean_anomaly: f64,
}

#[derive(Serialize, sqlx::FromRow)]
struct ObjectMetadata {
    object_type: String,
    rcs_size: String,
    country_code: String,
    launch_date: NaiveDate,
    launch_site: String,
}

fn cors(builder: http::response::Builder) -> http::response::Builder {
    builder
        .header("Access-Control-Allow-Origin", "*")
        .header("Access-Control-Allow-Methods", "GET, OPTIONS")
        .header("Access-Control-Allow-Headers", "*")
}

fn preflight() -> Response<Body> {
    cors(Response::builder().status(StatusCode::NO_CONTENT))
        .body(Body::empty())
        .unwrap()
}

pub async fn handler_get_object_metadata(req: Request<Body>) -> Response<Body> {
    if req.method() == Method::OPTIONS {
        return preflight();
    }

    let id = req
        .uri()
        .path()
        .trim_start_matches('/')
        .split('/')
        .next_back()
        .unwrap_or("")
        .to_string();
    println!("handler_get_object_metadata id: {}", id);

    if id.is_empty() {
        return cors(Response::builder().status(StatusCode::BAD_REQUEST))
            .body(Body::from("missing id in path"))
            .unwrap();
    }

    match get_object_metadata(id).await {
        Ok(val) => {
            let body = serde_json::to_string_pretty(&val).unwrap();

            cors(Response::builder().status(StatusCode::OK))
                .header("Content-type", "text/json")
                .body(Body::from(body))
                .unwrap()
        }
        Err(e) => cors(Response::builder().status(StatusCode::INTERNAL_SERVER_ERROR))
            .body(Body::from(e.to_string()))
            .unwrap(),
    }
}

pub async fn handler_get_all_objects(req: Request<Body>) -> Response<Body> {
    if req.method() == Method::OPTIONS {
        return preflight();
    }

    let all_objects_response: Result<AllObjectsResponse, sqlx::Error> = async {
        let database_config = get_database_config();
        let database_address = get_database_address(&database_config);
        let mut conn = sqlx::PgConnection::connect(database_address.as_str()).await?;
        let objects = get_all_objects(&mut conn).await?;
        let groups = get_groups(&mut conn).await?;
        Ok(AllObjectsResponse { objects, groups })
    }
    .await;

    match all_objects_response {
        Ok(val) => {
            let body = serde_json::to_string_pretty(&val).unwrap();
            cors(Response::builder().status(StatusCode::OK))
                .header("Content-type", "text/json")
                .body(Body::from(body))
                .unwrap()
        }
        Err(e) => cors(Response::builder().status(StatusCode::INTERNAL_SERVER_ERROR))
            .body(Body::from(e.to_string()))
            .unwrap(),
    }
}

async fn get_all_objects(conn: &mut sqlx::PgConnection) -> Result<Vec<ObjectEntry>, sqlx::Error> {
    let all_objects = sqlx::query_as::<_, ObjectEntry>(
        "SELECT
            id,
            norad_id,
            name,
            epoch,
            mean_motion,
            eccentricity,
            inclination,
            raan,
            arg_of_pericenter,
            mean_anomaly
        FROM public.objects",
    )
    .fetch_all(conn)
    .await?;

    Ok(all_objects)
}

async fn get_groups(conn: &mut sqlx::PgConnection) -> Result<GroupCollection, sqlx::Error> {
    let mut groups: GroupCollection = GroupCollection::new();
    get_explicit_groups(conn, &mut groups).await?;
    get_debris_group(conn, &mut groups).await?;
    get_last_30_days_launches_group(conn, &mut groups).await?;
    Ok(groups)
}

async fn get_explicit_groups(
    conn: &mut sqlx::PgConnection,
    groups: &mut GroupCollection,
) -> Result<(), sqlx::Error> {
    let entry_pairs =
        sqlx::query_as::<_, (i32, String)>(r#"SELECT norad_id, "group" FROM public.groups"#)
            .fetch_all(conn)
            .await?;

    for (norad_id, group) in entry_pairs {
        groups.entry(group).or_default().push(norad_id);
    }

    Ok(())
}

async fn get_debris_group(
    conn: &mut sqlx::PgConnection,
    groups: &mut GroupCollection,
) -> Result<(), sqlx::Error> {
    let entries: Vec<i32> =
        sqlx::query_scalar(r#"SELECT norad_id FROM public.objects WHERE object_type = 'DEBRIS'"#)
            .fetch_all(conn)
            .await?;

    groups.insert("debris".to_string(), entries);

    Ok(())
}

async fn get_last_30_days_launches_group(
    conn: &mut sqlx::PgConnection,
    groups: &mut GroupCollection,
) -> Result<(), sqlx::Error> {
    let entries: Vec<i32> =
        sqlx::query_scalar(r#"SELECT norad_id FROM public.objects WHERE launch_date >= CURRENT_DATE - INTERVAL '30 days'"#)
            .fetch_all(conn)
            .await?;

    groups.insert("last-30-days".to_string(), entries);

    Ok(())
}

async fn get_object_metadata(id: String) -> Result<ObjectMetadata, sqlx::Error> {
    let database_config = get_database_config();
    let database_address = get_database_address(&database_config);
    let mut conn = sqlx::PgConnection::connect(database_address.as_str()).await?;

    let object_metadata = sqlx::query_as::<_, ObjectMetadata>(
        "SELECT object_type, rcs_size, country_code, launch_date, launch_site FROM public.objects WHERE id = $1",
    )
    .bind(id)
    .fetch_one(&mut conn).await?;

    Ok(object_metadata)
}

fn get_database_config() -> DatabaseConfig {
    dotenvy::dotenv().ok();
    DatabaseConfig {
        host: std::env::var("DB_HOST").unwrap(),
        port: std::env::var("DB_PORT").unwrap(),
        name: std::env::var("DB_NAME").unwrap(),
        user: std::env::var("DB_USER").unwrap(),
        password: std::env::var("DB_PASSWORD").unwrap(),
    }
}

fn get_database_address(database_config: &DatabaseConfig) -> String {
    let url = format!(
        "postgres://{}:{}@{}:{}/{}",
        database_config.user,
        database_config.password,
        database_config.host,
        database_config.port,
        database_config.name
    );
    url
}
