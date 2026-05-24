const dbName = "taxi";
db = db.getSiblingDB(dbName);

const secretHash = "4317ddd8c6efe1208decf85c36bee03bd55d85aa7fe58be11a01256f36bb87cd";

db.users.deleteMany({});
db.drivers.deleteMany({});
db.rides.deleteMany({});

db.users.insertMany([
  {
    id: "u-1",
    login: "ivan",
    password_hash: secretHash,
    first_name: "Ivan",
    last_name: "Ivanov",
    email: "ivan@example.com",
    role: "passenger",
    rating: 4.8,
    created_at: ISODate("2026-01-10T10:00:00Z"),
    profile: { phone: "+79990000001", locale: "ru-RU" },
    saved_places: [
      { title: "Home", lat: 55.7558, lon: 37.6173 },
      { title: "Work", lat: 55.7522, lon: 37.6156 }
    ],
    payment_methods: ["card", "cash"]
  },
  {
    id: "u-2",
    login: "anna",
    password_hash: secretHash,
    first_name: "Anna",
    last_name: "Smirnova",
    email: "anna@example.com",
    role: "passenger",
    rating: 4.9,
    created_at: ISODate("2026-01-11T10:00:00Z"),
    profile: { phone: "+79990000002", locale: "ru-RU" },
    saved_places: [{ title: "Gym", lat: 55.76, lon: 37.62 }],
    payment_methods: ["card"]
  },
  {
    id: "u-3",
    login: "olga",
    password_hash: secretHash,
    first_name: "Olga",
    last_name: "Popova",
    email: "olga@example.com",
    role: "passenger",
    rating: 4.6,
    created_at: ISODate("2026-01-12T10:00:00Z"),
    profile: { phone: "+79990000003", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["cash"]
  },
  {
    id: "u-4",
    login: "alex",
    password_hash: secretHash,
    first_name: "Alexey",
    last_name: "Vasiliev",
    email: "alex@example.com",
    role: "passenger",
    rating: 4.7,
    created_at: ISODate("2026-01-13T10:00:00Z"),
    profile: { phone: "+79990000004", locale: "ru-RU" },
    saved_places: [{ title: "Airport", lat: 55.9726, lon: 37.4146 }],
    payment_methods: ["card", "bonus"]
  },
  {
    id: "u-5",
    login: "maria",
    password_hash: secretHash,
    first_name: "Maria",
    last_name: "Sokolova",
    email: "maria@example.com",
    role: "passenger",
    rating: 4.5,
    created_at: ISODate("2026-01-14T10:00:00Z"),
    profile: { phone: "+79990000005", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["card"]
  },
  {
    id: "u-6",
    login: "petr",
    password_hash: secretHash,
    first_name: "Petr",
    last_name: "Petrov",
    email: "petr@example.com",
    role: "driver",
    rating: 4.9,
    created_at: ISODate("2026-01-15T10:00:00Z"),
    profile: { phone: "+79990000006", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["card"]
  },
  {
    id: "u-7",
    login: "sergey",
    password_hash: secretHash,
    first_name: "Sergey",
    last_name: "Kuznetsov",
    email: "sergey@example.com",
    role: "driver",
    rating: 4.8,
    created_at: ISODate("2026-01-16T10:00:00Z"),
    profile: { phone: "+79990000007", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["card"]
  },
  {
    id: "u-8",
    login: "roman",
    password_hash: secretHash,
    first_name: "Roman",
    last_name: "Novikov",
    email: "roman@example.com",
    role: "driver",
    rating: 4.4,
    created_at: ISODate("2026-01-17T10:00:00Z"),
    profile: { phone: "+79990000008", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["cash"]
  },
  {
    id: "u-9",
    login: "dmitry",
    password_hash: secretHash,
    first_name: "Dmitry",
    last_name: "Fedorov",
    email: "dmitry@example.com",
    role: "driver",
    rating: 4.3,
    created_at: ISODate("2026-01-18T10:00:00Z"),
    profile: { phone: "+79990000009", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["card"]
  },
  {
    id: "u-10",
    login: "nikita",
    password_hash: secretHash,
    first_name: "Nikita",
    last_name: "Lebedev",
    email: "nikita@example.com",
    role: "driver",
    rating: 4.5,
    created_at: ISODate("2026-01-19T10:00:00Z"),
    profile: { phone: "+79990000010", locale: "ru-RU" },
    saved_places: [],
    payment_methods: ["card"]
  }
]);

db.drivers.insertMany([
  {
    id: "d-1",
    user_login: "petr",
    login: "petr",
    car: { model: "Hyundai Solaris", number: "A123AA777", class: "economy", year: 2021 },
    car_class: "economy",
    status: "FREE",
    rating: 4.9,
    shifts: [{ started_at: ISODate("2026-05-01T08:00:00Z"), finished_at: ISODate("2026-05-01T18:00:00Z") }]
  },
  {
    id: "d-2",
    user_login: "sergey",
    login: "sergey",
    car: { model: "Kia K5", number: "B234BB777", class: "comfort", year: 2022 },
    car_class: "comfort",
    status: "FREE",
    rating: 4.8,
    shifts: []
  },
  {
    id: "d-3",
    user_login: "roman",
    login: "roman",
    car: { model: "Toyota Camry", number: "C345CC777", class: "business", year: 2023 },
    car_class: "business",
    status: "OFFLINE",
    rating: 4.4,
    shifts: []
  },
  {
    id: "d-4",
    user_login: "dmitry",
    login: "dmitry",
    car: { model: "Skoda Octavia", number: "D456DD777", class: "comfort", year: 2020 },
    car_class: "comfort",
    status: "BUSY",
    rating: 4.3,
    shifts: []
  },
  {
    id: "d-5",
    user_login: "nikita",
    login: "nikita",
    car: { model: "Volkswagen Polo", number: "E567EE777", class: "economy", year: 2021 },
    car_class: "economy",
    status: "FREE",
    rating: 4.5,
    shifts: []
  },
  {
    id: "d-6",
    user_login: "ivan",
    login: "ivan",
    car: { model: "Mercedes E200", number: "F678FF777", class: "business", year: 2024 },
    car_class: "business",
    status: "FREE",
    rating: 4.6,
    shifts: []
  },
  {
    id: "d-7",
    user_login: "anna",
    login: "anna",
    car: { model: "Kia Rio", number: "G789GG777", class: "economy", year: 2019 },
    car_class: "economy",
    status: "OFFLINE",
    rating: 4.2,
    shifts: []
  },
  {
    id: "d-8",
    user_login: "olga",
    login: "olga",
    car: { model: "BMW 5 Series", number: "H890HH777", class: "business", year: 2022 },
    car_class: "business",
    status: "FREE",
    rating: 4.7,
    shifts: []
  },
  {
    id: "d-9",
    user_login: "alex",
    login: "alex",
    car: { model: "Haval Jolion", number: "K901KK777", class: "comfort", year: 2023 },
    car_class: "comfort",
    status: "FREE",
    rating: 4.4,
    shifts: []
  },
  {
    id: "d-10",
    user_login: "maria",
    login: "maria",
    car: { model: "Chery Tiggo 7", number: "M012MM777", class: "comfort", year: 2024 },
    car_class: "comfort",
    status: "BUSY",
    rating: 4.6,
    shifts: []
  }
]);

db.rides.insertMany([
  {
    id: "r-1",
    passenger_login: "ivan",
    driver_login: null,
    route: { from: { lat: 55.7558, lon: 37.6173 }, to: { lat: 55.7522, lon: 37.6156 } },
    car_class: "economy",
    status: "CREATED",
    price: NumberDecimal("250.00"),
    created_at: ISODate("2026-05-20T10:00:00Z"),
    events: [{ type: "created", at: ISODate("2026-05-20T10:00:00Z") }]
  },
  {
    id: "r-2",
    passenger_login: "anna",
    driver_login: "petr",
    route: { from: { lat: 55.76, lon: 37.62 }, to: { lat: 55.74, lon: 37.6 } },
    car_class: "economy",
    status: "ACCEPTED",
    price: NumberDecimal("310.00"),
    created_at: ISODate("2026-05-20T10:05:00Z"),
    events: [{ type: "created", at: ISODate("2026-05-20T10:05:00Z") }, { type: "accepted", at: ISODate("2026-05-20T10:06:00Z") }]
  },
  {
    id: "r-3",
    passenger_login: "olga",
    driver_login: "sergey",
    route: { from: { lat: 55.77, lon: 37.63 }, to: { lat: 55.73, lon: 37.59 } },
    car_class: "comfort",
    status: "COMPLETED",
    price: NumberDecimal("540.00"),
    created_at: ISODate("2026-05-19T10:00:00Z"),
    events: [{ type: "completed", at: ISODate("2026-05-19T10:30:00Z") }]
  },
  {
    id: "r-4",
    passenger_login: "alex",
    driver_login: "dmitry",
    route: { from: { lat: 55.75, lon: 37.61 }, to: { lat: 55.745, lon: 37.605 } },
    car_class: "comfort",
    status: "ACCEPTED",
    price: NumberDecimal("430.00"),
    created_at: ISODate("2026-05-20T09:00:00Z"),
    events: [{ type: "accepted", at: ISODate("2026-05-20T09:03:00Z") }]
  },
  {
    id: "r-5",
    passenger_login: "maria",
    driver_login: null,
    route: { from: { lat: 55.751, lon: 37.611 }, to: { lat: 55.746, lon: 37.606 } },
    car_class: "economy",
    status: "CREATED",
    price: NumberDecimal("210.00"),
    created_at: ISODate("2026-05-20T10:12:00Z"),
    events: [{ type: "created", at: ISODate("2026-05-20T10:12:00Z") }]
  },
  {
    id: "r-6",
    passenger_login: "ivan",
    driver_login: "roman",
    route: { from: { lat: 55.741, lon: 37.601 }, to: { lat: 55.731, lon: 37.591 } },
    car_class: "business",
    status: "COMPLETED",
    price: NumberDecimal("890.00"),
    created_at: ISODate("2026-05-18T10:00:00Z"),
    events: [{ type: "completed", at: ISODate("2026-05-18T10:40:00Z") }]
  },
  {
    id: "r-7",
    passenger_login: "anna",
    driver_login: "olga",
    route: { from: { lat: 55.735, lon: 37.585 }, to: { lat: 55.725, lon: 37.575 } },
    car_class: "business",
    status: "COMPLETED",
    price: NumberDecimal("920.00"),
    created_at: ISODate("2026-05-17T10:00:00Z"),
    events: [{ type: "completed", at: ISODate("2026-05-17T10:50:00Z") }]
  },
  {
    id: "r-8",
    passenger_login: "olga",
    driver_login: "nikita",
    route: { from: { lat: 55.722, lon: 37.572 }, to: { lat: 55.712, lon: 37.562 } },
    car_class: "economy",
    status: "COMPLETED",
    price: NumberDecimal("260.00"),
    created_at: ISODate("2026-05-16T10:00:00Z"),
    events: [{ type: "completed", at: ISODate("2026-05-16T10:25:00Z") }]
  },
  {
    id: "r-9",
    passenger_login: "alex",
    driver_login: null,
    route: { from: { lat: 55.711, lon: 37.561 }, to: { lat: 55.701, lon: 37.551 } },
    car_class: "comfort",
    status: "CREATED",
    price: NumberDecimal("470.00"),
    created_at: ISODate("2026-05-20T10:15:00Z"),
    events: [{ type: "created", at: ISODate("2026-05-20T10:15:00Z") }]
  },
  {
    id: "r-10",
    passenger_login: "maria",
    driver_login: "alex",
    route: { from: { lat: 55.7, lon: 37.55 }, to: { lat: 55.69, lon: 37.54 } },
    car_class: "comfort",
    status: "COMPLETED",
    price: NumberDecimal("510.00"),
    created_at: ISODate("2026-05-15T10:00:00Z"),
    events: [{ type: "completed", at: ISODate("2026-05-15T10:35:00Z") }]
  }
]);
