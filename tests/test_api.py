from __future__ import annotations

import uuid

import requests


# --------------------------------------------------------------------------- #
# /v1/login
# --------------------------------------------------------------------------- #

class TestLogin:

    def test_success(self, base_url, passenger):
        r = requests.post(
            f"{base_url}/v1/login",
            json={"login": passenger.login, "password": passenger.password},
        )
        assert r.status_code == 200
        body = r.json()
        assert body["token_type"] == "Bearer"
        assert body["expires_in"] >= 1
        assert body["token"].count(".") == 2  # header.payload.sig

    def test_missing_password_returns_400(self, base_url, passenger):
        r = requests.post(
            f"{base_url}/v1/login",
            json={"login": passenger.login},
        )
        assert r.status_code == 400
        assert r.json()["error"] == "validation_failed"

    def test_wrong_password_returns_401(self, base_url, passenger):
        r = requests.post(
            f"{base_url}/v1/login",
            json={"login": passenger.login, "password": "wrong"},
        )
        assert r.status_code == 401
        assert r.json()["error"] == "invalid_credentials"

    def test_invalid_json_returns_400(self, base_url):
        r = requests.post(
            f"{base_url}/v1/login",
            data="{not-json",
            headers={"Content-Type": "application/json"},
        )
        assert r.status_code == 400
        assert r.json()["error"] == "invalid_json"


# --------------------------------------------------------------------------- #
# /v1/users (POST + GET)
# --------------------------------------------------------------------------- #

class TestUsers:

    def test_create_success(self, base_url, run_id):
        login = f"newbie-{run_id}-{uuid.uuid4().hex[:6]}"
        r = requests.post(
            f"{base_url}/v1/users",
            json={
                "login": login,
                "password": "secret",
                "first_name": "Аня",
                "last_name": "Сидорова",
                "email": f"{login}@example.com",
            },
        )
        assert r.status_code == 201
        assert r.headers.get("Location", "").endswith(f"login={login}")
        body = r.json()
        assert body["login"] == login
        assert body["role"] == "passenger"
        assert body["id"].startswith("u-")

    def test_create_duplicate_returns_409(self, base_url, passenger):
        r = requests.post(
            f"{base_url}/v1/users",
            json={
                "login": passenger.login,
                "password": "secret",
                "first_name": "X",
                "last_name": "Y",
            },
        )
        assert r.status_code == 409
        assert r.json()["error"] == "login_taken"

    def test_create_missing_fields_returns_400(self, base_url):
        r = requests.post(
            f"{base_url}/v1/users",
            json={"login": "only-login"},
        )
        assert r.status_code == 400
        assert r.json()["error"] == "validation_failed"

    def test_get_by_login_success(self, base_url, passenger, auth_header):
        r = requests.get(
            f"{base_url}/v1/users",
            params={"login": passenger.login},
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 200
        assert r.json()["login"] == passenger.login

    def test_get_by_login_not_found(self, base_url, passenger, auth_header):
        r = requests.get(
            f"{base_url}/v1/users",
            params={"login": f"ghost-{uuid.uuid4().hex[:8]}"},
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 404
        assert r.json()["error"] == "user_not_found"

    def test_get_by_mask_success(self, base_url, passenger, auth_header):
        r = requests.get(
            f"{base_url}/v1/users",
            params={"nameMask": "Имя"},
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 200
        body = r.json()
        assert body["count"] >= 1
        assert any(u["login"] == passenger.login for u in body["items"])

    def test_get_without_filter_returns_400(
        self, base_url, passenger, auth_header
    ):
        r = requests.get(
            f"{base_url}/v1/users",
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 400
        assert r.json()["error"] == "validation_failed"

    def test_get_without_token_returns_401(self, base_url, passenger):
        r = requests.get(
            f"{base_url}/v1/users",
            params={"login": passenger.login},
        )
        assert r.status_code == 401

    def test_get_with_bad_token_returns_401(self, base_url, passenger):
        r = requests.get(
            f"{base_url}/v1/users",
            params={"login": passenger.login},
            headers={"Authorization": "Bearer not-a-real-jwt"},
        )
        assert r.status_code == 401


# --------------------------------------------------------------------------- #
# /v1/drivers
# --------------------------------------------------------------------------- #

class TestDrivers:

    def test_register_success(self, base_url, run_id, auth_header):
        # Fresh user → register self as a driver.
        login = f"driver-{run_id}-{uuid.uuid4().hex[:6]}"
        requests.post(
            f"{base_url}/v1/users",
            json={
                "login": login, "password": "secret",
                "first_name": "Д", "last_name": "В",
            },
        ).raise_for_status()
        token = requests.post(
            f"{base_url}/v1/login",
            json={"login": login, "password": "secret"},
        ).json()["token"]

        r = requests.post(
            f"{base_url}/v1/drivers",
            headers=auth_header(token),
            json={
                "car_model": "Lada Vesta",
                "car_number": f"С000{uuid.uuid4().hex[:3].upper()}",
                "car_class": "economy",
            },
        )
        assert r.status_code == 201
        body = r.json()
        assert body["login"] == login
        assert body["id"].startswith("d-")
        assert body["status"] == "OFFLINE"

    def test_register_missing_car_returns_400(
        self, base_url, passenger, auth_header
    ):
        r = requests.post(
            f"{base_url}/v1/drivers",
            headers=auth_header(passenger.token),
            json={"car_class": "economy"},  # no car_model / car_number
        )
        assert r.status_code == 400
        assert r.json()["error"] == "validation_failed"

    def test_register_duplicate_returns_409(
        self, base_url, driver, auth_header
    ):
        r = requests.post(
            f"{base_url}/v1/drivers",
            headers=auth_header(driver.token),
            json={"car_model": "Solaris", "car_number": "X1"},
        )
        assert r.status_code == 409
        assert r.json()["error"] == "driver_exists"

    def test_register_without_token_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/v1/drivers",
            json={"car_model": "X", "car_number": "Y"},
        )
        assert r.status_code == 401


# --------------------------------------------------------------------------- #
# /v1/rides — create + list active
# --------------------------------------------------------------------------- #

class TestRidesCreate:

    def test_create_success(self, base_url, passenger, auth_header):
        r = requests.post(
            f"{base_url}/v1/rides",
            headers=auth_header(passenger.token),
            json={
                "from": {"lat": 55.7558, "lon": 37.6173},
                "to":   {"lat": 55.7522, "lon": 37.6156},
                "car_class": "comfort",
            },
        )
        assert r.status_code == 201
        body = r.json()
        assert body["passenger_login"] == passenger.login
        assert body["status"] == "CREATED"
        assert body["car_class"] == "comfort"
        assert body["price"] > 0
        assert body["driver_login"] is None

    def test_create_invalid_json_returns_400(
        self, base_url, passenger, auth_header
    ):
        r = requests.post(
            f"{base_url}/v1/rides",
            headers={**auth_header(passenger.token),
                     "Content-Type": "application/json"},
            data="not json",
        )
        assert r.status_code == 400

    def test_create_without_token_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/v1/rides",
            json={"from": {"lat": 0, "lon": 0}, "to": {"lat": 0, "lon": 0}},
        )
        assert r.status_code == 401


class TestRidesList:

    def test_list_active_as_driver(
        self, base_url, driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        r = requests.get(
            f"{base_url}/v1/rides",
            params={"status": "active"},
            headers=auth_header(driver.token),
        )
        assert r.status_code == 200
        body = r.json()
        assert any(ride["id"] == ride_id for ride in body["items"])

    def test_list_as_passenger_returns_403(
        self, base_url, passenger, auth_header
    ):
        r = requests.get(
            f"{base_url}/v1/rides",
            params={"status": "active"},
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 403
        assert r.json()["error"] == "forbidden"

    def test_list_unsupported_status_returns_400(
        self, base_url, driver, auth_header
    ):
        r = requests.get(
            f"{base_url}/v1/rides",
            params={"status": "completed"},
            headers=auth_header(driver.token),
        )
        assert r.status_code == 400

    def test_list_without_token_returns_401(self, base_url):
        r = requests.get(
            f"{base_url}/v1/rides", params={"status": "active"}
        )
        assert r.status_code == 401


# --------------------------------------------------------------------------- #
# /v1/rides/{id}/accept + /complete
# --------------------------------------------------------------------------- #

class TestRideAccept:

    def test_accept_success(
        self, base_url, driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        r = requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(driver.token),
        )
        assert r.status_code == 200
        body = r.json()
        assert body["status"] == "ACCEPTED"
        assert body["driver_login"] == driver.login

    def test_accept_twice_returns_409(
        self, base_url, driver, second_driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        ok = requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(driver.token),
        )
        assert ok.status_code == 200
        again = requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(second_driver.token),
        )
        assert again.status_code == 409
        assert again.json()["error"] == "ride_not_available"

    def test_accept_unknown_ride_returns_404(
        self, base_url, driver, auth_header
    ):
        r = requests.post(
            f"{base_url}/v1/rides/r-nonexistent/accept",
            headers=auth_header(driver.token),
        )
        assert r.status_code == 404
        assert r.json()["error"] == "ride_not_found"

    def test_accept_as_passenger_returns_403(
        self, base_url, passenger, create_ride, auth_header
    ):
        ride_id = create_ride()
        r = requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 403
        assert r.json()["error"] == "forbidden"

    def test_accept_without_token_returns_401(self, base_url, create_ride):
        ride_id = create_ride()
        r = requests.post(f"{base_url}/v1/rides/{ride_id}/accept")
        assert r.status_code == 401


class TestRideComplete:

    def test_complete_success(
        self, base_url, driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(driver.token),
        ).raise_for_status()

        r = requests.post(
            f"{base_url}/v1/rides/{ride_id}/complete",
            headers=auth_header(driver.token),
        )
        assert r.status_code == 200
        assert r.json()["status"] == "COMPLETED"

    def test_complete_unaccepted_returns_409(
        self, base_url, driver, create_ride, auth_header
    ):
        ride_id = create_ride()  # still CREATED, not ACCEPTED
        r = requests.post(
            f"{base_url}/v1/rides/{ride_id}/complete",
            headers=auth_header(driver.token),
        )
        assert r.status_code == 409
        assert r.json()["error"] == "invalid_state"

    def test_complete_by_other_driver_returns_409(
        self, base_url, driver, second_driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(driver.token),
        ).raise_for_status()

        r = requests.post(
            f"{base_url}/v1/rides/{ride_id}/complete",
            headers=auth_header(second_driver.token),
        )
        assert r.status_code == 409

    def test_complete_unknown_returns_404(
        self, base_url, driver, auth_header
    ):
        r = requests.post(
            f"{base_url}/v1/rides/r-nope/complete",
            headers=auth_header(driver.token),
        )
        assert r.status_code == 404


# --------------------------------------------------------------------------- #
# /v1/users/{login}/rides
# --------------------------------------------------------------------------- #

class TestRidesHistory:

    def test_history_success(
        self, base_url, passenger, driver, create_ride, auth_header
    ):
        ride_id = create_ride()
        requests.post(
            f"{base_url}/v1/rides/{ride_id}/accept",
            headers=auth_header(driver.token),
        ).raise_for_status()

        r = requests.get(
            f"{base_url}/v1/users/{passenger.login}/rides",
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 200
        body = r.json()
        assert body["count"] >= 1
        assert any(ride["id"] == ride_id for ride in body["items"])

    def test_history_unknown_user_returns_404(
        self, base_url, passenger, auth_header
    ):
        r = requests.get(
            f"{base_url}/v1/users/no-such-user-{uuid.uuid4().hex[:6]}/rides",
            headers=auth_header(passenger.token),
        )
        assert r.status_code == 404
        assert r.json()["error"] == "user_not_found"

    def test_history_without_token_returns_401(self, base_url, passenger):
        r = requests.get(f"{base_url}/v1/users/{passenger.login}/rides")
        assert r.status_code == 401
