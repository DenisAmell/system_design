-- 1. Create user
INSERT INTO users (id, login, password_hash, first_name, last_name, email, role)
VALUES ($1, $2, $3, $4, $5, $6, 'passenger');

-- 2. Find user by login
SELECT id, login, first_name, last_name, email, role
FROM users
WHERE login = $1
LIMIT 1;

-- 3. Find users by first/last name mask
SELECT id, login, first_name, last_name, email, role
FROM users
WHERE first_name ILIKE $1 || '%' OR last_name ILIKE $1 || '%'
ORDER BY login;

-- 4. Register driver
UPDATE users
SET role = 'driver'
WHERE login = $1;

INSERT INTO drivers (id, user_id, login, car_model, car_number, car_class, status)
VALUES ($2, $3, $1, $4, $5, $6, 'OFFLINE');

-- 5. Create ride order
INSERT INTO rides (
    id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
    car_class, status, price, created_at
)
VALUES ($1, $2, NULL, $3, $4, $5, $6, $7, 'CREATED', $8, now());

-- 6. List active ride orders
SELECT id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
       car_class, status, price, created_at
FROM rides
WHERE status = 'CREATED'
ORDER BY created_at DESC;

-- 7. Accept ride by driver
UPDATE rides
SET status = 'ACCEPTED', driver_login = $2
WHERE id = $1 AND status = 'CREATED';

UPDATE drivers
SET status = 'BUSY'
WHERE login = $2;

-- 8. Complete ride
UPDATE rides
SET status = 'COMPLETED'
WHERE id = $1 AND status = 'ACCEPTED' AND driver_login = $2;

UPDATE drivers
SET status = 'FREE'
WHERE login = $2;

-- 9. User ride history
SELECT id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
       car_class, status, price, created_at
FROM rides
WHERE passenger_login = $1 OR driver_login = $1
ORDER BY created_at DESC;

EXPLAIN (ANALYZE, BUFFERS)
SELECT id, passenger_login, driver_login, created_at
FROM rides
WHERE status = 'CREATED'
ORDER BY created_at DESC
LIMIT 50;

EXPLAIN (ANALYZE, BUFFERS)
SELECT id, status, created_at
FROM rides
WHERE passenger_login = 'ivan'
ORDER BY created_at DESC
LIMIT 100;

EXPLAIN (ANALYZE, BUFFERS)
UPDATE rides
SET status = 'ACCEPTED', driver_login = 'petr'
WHERE id = 'r-1' AND status = 'CREATED';
