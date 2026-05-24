INSERT INTO users (id, login, password_hash, first_name, last_name, email, role) VALUES
('u-1',  'ivan',    'hash-1',  'Ivan',    'Ivanov',     'ivan@example.com',    'passenger'),
('u-2',  'anna',    'hash-2',  'Anna',    'Smirnova',   'anna@example.com',    'passenger'),
('u-3',  'olga',    'hash-3',  'Olga',    'Popova',     'olga@example.com',    'passenger'),
('u-4',  'alex',    'hash-4',  'Alexey',  'Vasiliev',   'alex@example.com',    'passenger'),
('u-5',  'maria',   'hash-5',  'Maria',   'Sokolova',   'maria@example.com',   'passenger'),
('u-6',  'petr',    'hash-6',  'Petr',    'Petrov',     'petr@example.com',    'driver'),
('u-7',  'sergey',  'hash-7',  'Sergey',  'Kuznetsov',  'sergey@example.com',  'driver'),
('u-8',  'roman',   'hash-8',  'Roman',   'Novikov',    'roman@example.com',   'driver'),
('u-9',  'dmitry',  'hash-9',  'Dmitry',  'Fedorov',    'dmitry@example.com',  'driver'),
('u-10', 'nikita',  'hash-10', 'Nikita',  'Lebedev',    'nikita@example.com',  'driver'),
('u-11', 'daria',   'hash-11', 'Daria',   'Kozlova',    'daria@example.com',   'driver'),
('u-12', 'elena',   'hash-12', 'Elena',   'Morozova',   'elena@example.com',   'driver'),
('u-13', 'maxim',   'hash-13', 'Maxim',   'Orlov',      'maxim@example.com',   'driver'),
('u-14', 'kirill',  'hash-14', 'Kirill',  'Volkov',     'kirill@example.com',  'driver'),
('u-15', 'svetlana','hash-15', 'Svetlana','Pavlova',    'svetlana@example.com','driver')
ON CONFLICT (login) DO NOTHING;

INSERT INTO drivers (id, user_id, login, car_model, car_number, car_class, status) VALUES
('d-1',  'u-6',  'petr',     'Hyundai Solaris', 'A123AA777', 'economy',  'FREE'),
('d-2',  'u-7',  'sergey',   'Kia K5',          'B234BB777', 'comfort',  'FREE'),
('d-3',  'u-8',  'roman',    'Toyota Camry',    'C345CC777', 'business', 'OFFLINE'),
('d-4',  'u-9',  'dmitry',   'Skoda Octavia',   'D456DD777', 'comfort',  'BUSY'),
('d-5',  'u-10', 'nikita',   'Volkswagen Polo', 'E567EE777', 'economy',  'FREE'),
('d-6',  'u-11', 'daria',    'Mercedes E200',   'F678FF777', 'business', 'FREE'),
('d-7',  'u-12', 'elena',    'Kia Rio',         'G789GG777', 'economy',  'OFFLINE'),
('d-8',  'u-13', 'maxim',    'BMW 5 Series',    'H890HH777', 'business', 'FREE'),
('d-9',  'u-14', 'kirill',   'Haval Jolion',    'K901KK777', 'comfort',  'FREE'),
('d-10', 'u-15', 'svetlana', 'Chery Tiggo 7',   'M012MM777', 'comfort',  'BUSY')
ON CONFLICT (login) DO NOTHING;

INSERT INTO rides (
    id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
    car_class, status, price, created_at
) VALUES
('r-1',  'ivan',  NULL,      55.7558, 37.6173, 55.7522, 37.6156, 'economy',  'CREATED',   250.00, now() - interval '20 minutes'),
('r-2',  'anna',  'petr',    55.7600, 37.6200, 55.7400, 37.6000, 'economy',  'ACCEPTED',  310.00, now() - interval '18 minutes'),
('r-3',  'olga',  'sergey',  55.7700, 37.6300, 55.7300, 37.5900, 'comfort',  'COMPLETED', 540.00, now() - interval '2 hours'),
('r-4',  'alex',  'dmitry',  55.7500, 37.6100, 55.7450, 37.6050, 'comfort',  'ACCEPTED',  430.00, now() - interval '1 hours'),
('r-5',  'maria', NULL,      55.7510, 37.6110, 55.7460, 37.6060, 'economy',  'CREATED',   210.00, now() - interval '12 minutes'),
('r-6',  'ivan',  'roman',   55.7410, 37.6010, 55.7310, 37.5910, 'business', 'COMPLETED', 890.00, now() - interval '1 days'),
('r-7',  'anna',  'daria',   55.7350, 37.5850, 55.7250, 37.5750, 'business', 'COMPLETED', 920.00, now() - interval '3 days'),
('r-8',  'olga',  'nikita',  55.7220, 37.5720, 55.7120, 37.5620, 'economy',  'COMPLETED', 260.00, now() - interval '5 days'),
('r-9',  'alex',  NULL,      55.7110, 37.5610, 55.7010, 37.5510, 'comfort',  'CREATED',   470.00, now() - interval '6 minutes'),
('r-10', 'maria', 'kirill',  55.7000, 37.5500, 55.6900, 37.5400, 'comfort',  'COMPLETED', 510.00, now() - interval '7 days'),
('r-11', 'ivan',  'maxim',   55.7800, 37.6500, 55.7600, 37.6200, 'business', 'COMPLETED', 790.00, now() - interval '10 days'),
('r-12', 'anna',  NULL,      55.7900, 37.6600, 55.7700, 37.6300, 'economy',  'CREATED',   280.00, now() - interval '3 minutes')
ON CONFLICT (id) DO NOTHING;
