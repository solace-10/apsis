# Essential documentation

https://developers.cloudflare.com/r2/api/workers/workers-api-usage

# Authentication

The worker expects the shared key set in the environment variable FORGE_AUTH_KEY_SECRET to be sent for PUT requests.

# Development

A local development server can be run via `npx wrangler dev`.

# Deployment

A new version of the worker can be deployed by running `npx wrangler deploy`.
