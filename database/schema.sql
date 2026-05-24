CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE TABLE IF NOT EXISTS users (
    id            TEXT PRIMARY KEY,
    login         TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    first_name    TEXT NOT NULL,
    last_name     TEXT NOT NULL,
    email         TEXT,
    role          TEXT NOT NULL DEFAULT 'passenger'
                  CHECK (role IN ('passenger', 'driver'))
);

CREATE TABLE IF NOT EXISTS drivers (
    id         TEXT PRIMARY KEY,
    user_id    TEXT NOT NULL UNIQUE,
    login      TEXT NOT NULL UNIQUE,
    car_model  TEXT NOT NULL,
    car_number TEXT NOT NULL UNIQUE,
    car_class  TEXT NOT NULL
               CHECK (car_class IN ('economy', 'comfort', 'business')),
    status     TEXT NOT NULL DEFAULT 'OFFLINE'
               CHECK (status IN ('FREE', 'BUSY', 'OFFLINE')),
    CONSTRAINT fk_drivers_user_id
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_drivers_login
        FOREIGN KEY (login) REFERENCES users(login) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS rides (
    id              TEXT PRIMARY KEY,
    passenger_login TEXT NOT NULL,
    driver_login    TEXT,
    from_lat        DOUBLE PRECISION NOT NULL
                    CHECK (from_lat BETWEEN -90 AND 90),
    from_lon        DOUBLE PRECISION NOT NULL
                    CHECK (from_lon BETWEEN -180 AND 180),
    to_lat          DOUBLE PRECISION NOT NULL
                    CHECK (to_lat BETWEEN -90 AND 90),
    to_lon          DOUBLE PRECISION NOT NULL
                    CHECK (to_lon BETWEEN -180 AND 180),
    car_class       TEXT NOT NULL
                    CHECK (car_class IN ('economy', 'comfort', 'business')),
    status          TEXT NOT NULL DEFAULT 'CREATED'
                    CHECK (status IN ('CREATED', 'ACCEPTED', 'COMPLETED', 'CANCELLED')),
    price           NUMERIC(10,2) NOT NULL CHECK (price >= 0),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT fk_rides_passenger
        FOREIGN KEY (passenger_login) REFERENCES users(login),
    CONSTRAINT fk_rides_driver
        FOREIGN KEY (driver_login) REFERENCES drivers(login)
);

CREATE INDEX IF NOT EXISTS users_first_name_trgm_idx
    ON users USING gin (first_name gin_trgm_ops);

CREATE INDEX IF NOT EXISTS users_last_name_trgm_idx
    ON users USING gin (last_name gin_trgm_ops);

CREATE INDEX IF NOT EXISTS drivers_status_class_idx
    ON drivers(status, car_class);

CREATE INDEX IF NOT EXISTS rides_passenger_login_idx
    ON rides(passenger_login);

CREATE INDEX IF NOT EXISTS rides_driver_login_idx
    ON rides(driver_login);

CREATE INDEX IF NOT EXISTS rides_status_created_at_idx
    ON rides(status, created_at DESC);

CREATE INDEX IF NOT EXISTS rides_passenger_created_at_idx
    ON rides(passenger_login, created_at DESC);

CREATE INDEX IF NOT EXISTS rides_driver_created_at_idx
    ON rides(driver_login, created_at DESC);

CREATE INDEX IF NOT EXISTS rides_id_status_idx
    ON rides(id, status);
