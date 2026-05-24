const dbName = "taxi";
db = db.getSiblingDB(dbName);

const usersValidator = {
  $jsonSchema: {
    bsonType: "object",
    required: ["id", "login", "password_hash", "first_name", "last_name", "role", "created_at"],
    properties: {
      id: {
        bsonType: "string",
        pattern: "^u-[0-9]+$"
      },
      login: {
        bsonType: "string",
        minLength: 3,
        maxLength: 64,
        pattern: "^[a-zA-Z0-9_.-]+$"
      },
      password_hash: {
        bsonType: "string",
        minLength: 64,
        maxLength: 64
      },
      first_name: {
        bsonType: "string",
        minLength: 1
      },
      last_name: {
        bsonType: "string",
        minLength: 1
      },
      email: {
        bsonType: "string"
      },
      role: {
        enum: ["passenger", "driver"]
      },
      rating: {
        bsonType: ["double", "int", "decimal"],
        minimum: 0,
        maximum: 5
      },
      created_at: {
        bsonType: "date"
      },
      profile: {
        bsonType: "object"
      },
      saved_places: {
        bsonType: "array"
      },
      payment_methods: {
        bsonType: "array"
      }
    }
  }
};

if (!db.getCollectionNames().includes("users")) {
  db.createCollection("users", {
    validator: usersValidator,
    validationLevel: "strict",
    validationAction: "error"
  });
} else {
  db.runCommand({
    collMod: "users",
    validator: usersValidator,
    validationLevel: "strict",
    validationAction: "error"
  });
}

db.users.createIndex({ login: 1 }, { unique: true });
db.users.createIndex({ first_name: 1, last_name: 1 });
db.drivers.createIndex({ login: 1 }, { unique: true });
db.drivers.createIndex({ status: 1, car_class: 1 });
db.rides.createIndex({ status: 1, created_at: -1 });
db.rides.createIndex({ passenger_login: 1, created_at: -1 });
db.rides.createIndex({ driver_login: 1, created_at: -1 });

try {
  db.users.insertOne({
    id: "bad-user",
    login: "!",
    password_hash: "too-short",
    first_name: "",
    last_name: "",
    role: "admin",
    created_at: new Date()
  });
} catch (e) {
  print("Validation check passed: invalid user was rejected");
}
