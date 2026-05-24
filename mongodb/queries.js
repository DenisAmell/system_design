const dbName = "taxi";
db = db.getSiblingDB(dbName);

// CREATE: create a passenger.
db.users.insertOne({
  id: "u-101",
  login: "new-passenger",
  password_hash: "4317ddd8c6efe1208decf85c36bee03bd55d85aa7fe58be11a01256f36bb87cd",
  first_name: "New",
  last_name: "Passenger",
  email: "new-passenger@example.com",
  role: "passenger",
  rating: 5,
  created_at: new Date(),
  profile: { phone: "+79991112233", locale: "ru-RU" },
  saved_places: [],
  payment_methods: ["card"]
});

// READ with $eq: find user by login.
db.users.findOne({ login: { $eq: "ivan" } });

// READ with regex and $or: search by first/last name mask.
db.users.find({
  $or: [
    { first_name: { $regex: "^Iv", $options: "i" } },
    { last_name: { $regex: "^Iv", $options: "i" } }
  ]
}).sort({ login: 1 });

// READ with $in and $ne: available economy/comfort drivers.
db.drivers.find({
  status: { $ne: "OFFLINE" },
  car_class: { $in: ["economy", "comfort"] }
});

// READ with $and, $gt, $lt: recent rides in price range.
db.rides.find({
  $and: [
    { price: { $gt: NumberDecimal("200.00") } },
    { price: { $lt: NumberDecimal("600.00") } },
    { created_at: { $gt: ISODate("2026-05-15T00:00:00Z") } }
  ]
});

// CREATE: create a ride order.
db.rides.insertOne({
  id: "r-101",
  passenger_login: "ivan",
  driver_login: null,
  route: {
    from: { lat: 55.7558, lon: 37.6173 },
    to: { lat: 55.7522, lon: 37.6156 }
  },
  car_class: "economy",
  status: "CREATED",
  price: NumberDecimal("250.00"),
  created_at: new Date(),
  events: [{ type: "created", at: new Date() }]
});

// UPDATE: accept ride by driver and append event to embedded events array.
db.rides.updateOne(
  { id: "r-101", status: "CREATED" },
  {
    $set: { status: "ACCEPTED", driver_login: "petr" },
    $push: { events: { type: "accepted", by: "petr", at: new Date() } }
  }
);

// UPDATE: complete ride.
db.rides.updateOne(
  { id: "r-101", status: "ACCEPTED", driver_login: "petr" },
  {
    $set: { status: "COMPLETED" },
    $push: { events: { type: "completed", by: "petr", at: new Date() } }
  }
);

// UPDATE with $addToSet: add a payment method without duplicates.
db.users.updateOne(
  { login: "ivan" },
  { $addToSet: { payment_methods: "bonus" } }
);

// UPDATE with $pull: remove a saved place by title.
db.users.updateOne(
  { login: "ivan" },
  { $pull: { saved_places: { title: "Work" } } }
);

// READ: active orders.
db.rides.find({ status: "CREATED" }).sort({ created_at: -1 });

// READ: ride history for a user as passenger or driver.
db.rides.find({
  $or: [
    { passenger_login: "ivan" },
    { driver_login: "ivan" }
  ]
}).sort({ created_at: -1 });

// DELETE: cancel and remove obsolete created ride.
db.rides.deleteOne({ id: "r-101", status: { $in: ["CREATED", "COMPLETED"] } });

// Validation test: this must fail because login, role and password_hash violate schema.
try {
  db.users.insertOne({
    id: "invalid",
    login: "!",
    password_hash: "short",
    first_name: "",
    last_name: "",
    role: "admin",
    created_at: new Date()
  });
} catch (e) {
  print("Validation check passed: invalid user was rejected");
}
