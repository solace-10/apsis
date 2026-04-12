mod handler;

use axum::{Router, routing::any};

#[tokio::main]
async fn main() {
    let app = Router::new()
        .route("/get_all_objects", any(handler::handler_get_all_objects))
        .route(
            "/get_object_metadata/{id}",
            any(handler::handler_get_object_metadata),
        );
    let addr = "0.0.0.0:8080";
    println!("Listening on {addr}");
    let listener = tokio::net::TcpListener::bind(addr).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}
