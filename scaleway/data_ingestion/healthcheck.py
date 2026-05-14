"""
Healthcheck module for pinging healthchecks.io HTTP API.
"""

import logging

import requests

logger = logging.getLogger(__name__)


class HealthCheck:
    def __init__(self, url: str):
        self.url = url.rstrip("/")

    def start(self):
        self._ping(f"{self.url}/start", "start")

    def success(self):
        self._ping(self.url, "success")

    def fail(self):
        self._ping(f"{self.url}/fail", "fail")

    def _ping(self, url, signal):
        try:
            resp = requests.get(url, timeout=10)
            logger.info(f"Healthcheck {signal} ping: HTTP {resp.status_code}")
        except requests.RequestException as e:
            logger.warning(f"Healthcheck {signal} ping failed: {e}")
