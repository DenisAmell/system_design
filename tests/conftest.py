from __future__ import annotations

import os
import time
import uuid
from dataclasses import dataclass

import pytest
import requests

BASE_URL = os.environ.get("BASE_URL", "http://localhost:8080")
WAIT_FOR_SERVICE_SECS = float(os.environ.get("WAIT_FOR_SERVICE_SECS", "30"))


@dataclass
class Account:
    login: str
    password: str
    token: str
    user_id: str


def _wait_for_service() -> None:
    """Probe /v1/users (any 4xx is fine — means the server is up)."""
    deadline = time.monotonic() + WAIT_FOR_SERVICE_SECS
    last_err: Exception | None = None
    while time.monotonic() < deadline:
        try:
            r = requests.get(f"{BASE_URL}/v1/users", timeout=2)
            if r.status_code < 500:
                return
        except requests.RequestException as e:
            last_err = e
        time.sleep(0.5)
    raise RuntimeError(
        f"taxi-service is not reachable at {BASE_URL} "
        f"(last error: {last_err})"
    )


@pytest.fixture(scope="session", autouse=True)
def _service_up() -> None:
    _wait_for_service()


@pytest.fixture(scope="session")
def base_url() -> str:
    return BASE_URL


@pytest.fixture(scope="session")
def run_id() -> str:
    """Short unique suffix shared by all logins in this pytest run."""
    return uuid.uuid4().hex[:8]


# --------------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------------- #

def _register(base_url: str, login: str, password: str = "secret") -> str:
    r = requests.post(
        f"{base_url}/v1/users",
        json={
            "login": login,
            "password": password,
            "first_name": "Имя",
            "last_name":  "Фамилия",
            "email": f"{login}@example.com",
        },
        timeout=5,
    )
    assert r.status_code == 201, r.text
    return r.json()["id"]


def _login(base_url: str, login: str, password: str = "secret") -> str:
    r = requests.post(
        f"{base_url}/v1/login",
        json={"login": login, "password": password},
        timeout=5,
    )
    assert r.status_code == 200, r.text
    return r.json()["token"]


def _make_account(base_url: str, login: str) -> Account:
    uid = _register(base_url, login)
    token = _login(base_url, login)
    return Account(login=login, password="secret", token=token, user_id=uid)


# --------------------------------------------------------------------------- #
# accounts
# --------------------------------------------------------------------------- #

@pytest.fixture(scope="session")
def passenger(base_url: str, run_id: str) -> Account:
    return _make_account(base_url, f"ivan-{run_id}")


@pytest.fixture(scope="session")
def driver(base_url: str, run_id: str) -> Account:
    acc = _make_account(base_url, f"petr-{run_id}")
    r = requests.post(
        f"{base_url}/v1/drivers",
        headers={"Authorization": f"Bearer {acc.token}"},
        json={
            "car_model": "Hyundai Solaris",
            "car_number": f"А000{run_id[:3].upper()}",
            "car_class": "economy",
        },
        timeout=5,
    )
    assert r.status_code == 201, r.text
    return acc


@pytest.fixture
def second_driver(base_url: str, run_id: str) -> Account:
    """A second driver created per-test (for cross-driver conflict checks)."""
    suffix = uuid.uuid4().hex[:6]
    acc = _make_account(base_url, f"sergey-{run_id}-{suffix}")
    r = requests.post(
        f"{base_url}/v1/drivers",
        headers={"Authorization": f"Bearer {acc.token}"},
        json={
            "car_model": "Kia Rio",
            "car_number": f"В000{suffix[:3].upper()}",
            "car_class": "economy",
        },
        timeout=5,
    )
    assert r.status_code == 201, r.text
    return acc


# --------------------------------------------------------------------------- #
# helpers exposed to tests
# --------------------------------------------------------------------------- #

@pytest.fixture
def auth_header():
    def _make(token: str) -> dict[str, str]:
        return {"Authorization": f"Bearer {token}"}
    return _make


@pytest.fixture
def create_ride(base_url: str, passenger: Account, auth_header):
    """Factory: returns a fresh ride id each call."""
    def _make() -> str:
        r = requests.post(
            f"{base_url}/v1/rides",
            headers=auth_header(passenger.token),
            json={
                "from": {"lat": 55.7558, "lon": 37.6173},
                "to":   {"lat": 55.7522, "lon": 37.6156},
                "car_class": "economy",
            },
            timeout=5,
        )
        assert r.status_code == 201, r.text
        return r.json()["id"]
    return _make
