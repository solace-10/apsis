use axum::extract::Path;
use axum::{body::Body, extract::Request, response::Response};
use chrono::NaiveDate;
use http::StatusCode;
use serde::Serialize;
use sqlx::Connection;

struct DatabaseConfig {
    host: String,
    port: String,
    name: String,
    user: String,
    password: String,
}

#[derive(Serialize, sqlx::FromRow)]
struct ObjectMetadata {
    object_type: String,
    rcs_size: String,
    country_code: String,
    launch_date: NaiveDate,
    launch_site: String,
}

pub async fn handler_get_object_metadata(
    Path(id): Path<String>,
    _req: Request<Body>,
) -> Response<Body> {
    println!("handler_get_object_metadata id: {}", id);
    let query_result = match get_object_metadata(id).await {
        Ok(val) => {
            let body = serde_json::to_string_pretty(&val).unwrap();

            Response::builder()
                .status(StatusCode::OK)
                .header("Content-type", "text/plain")
                .body(Body::from(body))
                .unwrap()
        }
        Err(e) => Response::builder()
            .status(StatusCode::INTERNAL_SERVER_ERROR)
            .body(Body::from(e.to_string()))
            .unwrap(),
    };
    query_result
}

pub async fn handler_get_all_objects(_req: Request<Body>) -> Response<Body> {
    println!("get_all_objects");
    Response::builder()
        .status(StatusCode::OK)
        .header("Content-type", "text/plain")
        .body(Body::from("Hello from rust - get_all_objects"))
        .unwrap()
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
