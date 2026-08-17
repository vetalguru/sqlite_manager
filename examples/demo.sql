-- Demo database for sqlite_manager: a broad schema exercising every
-- SQLite storage class and many declared column types, plus views,
-- indexes, a trigger, and sample rows. Build it with:
--
--   sqlite-manager --batch examples/demo.db < examples/demo.sql
--   # or: sqlite3 examples/demo.db < examples/demo.sql
--
-- then open examples/demo.db in the GUI or the CLI.

PRAGMA foreign_keys = ON;

-- Classic integer PK + text/date/boolean columns, with a trigger below.
CREATE TABLE users (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    name       TEXT    NOT NULL,
    email      TEXT    UNIQUE,
    age        INTEGER,
    is_active  BOOLEAN NOT NULL DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME
);

-- Real/numeric money and measurements, a CHECK constraint.
CREATE TABLE products (
    id          INTEGER PRIMARY KEY,
    sku         TEXT    NOT NULL UNIQUE,
    name        TEXT    NOT NULL,
    price       NUMERIC NOT NULL CHECK (price >= 0),
    weight_kg   REAL,
    in_stock    INTEGER DEFAULT 0,
    description TEXT
);

-- Foreign key to users, a status with a CHECK, a NUMERIC total.
CREATE TABLE orders (
    id         INTEGER PRIMARY KEY,
    user_id    INTEGER NOT NULL REFERENCES users(id),
    total      NUMERIC DEFAULT 0,
    status     TEXT    DEFAULT 'new'
                       CHECK (status IN ('new', 'paid', 'shipped', 'cancelled')),
    ordered_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Composite primary key and two foreign keys.
CREATE TABLE order_items (
    order_id   INTEGER NOT NULL REFERENCES orders(id),
    product_id INTEGER NOT NULL REFERENCES products(id),
    quantity   INTEGER NOT NULL CHECK (quantity > 0),
    unit_price REAL    NOT NULL,
    PRIMARY KEY (order_id, product_id)
);

-- Self-referencing hierarchy.
CREATE TABLE categories (
    id        INTEGER PRIMARY KEY,
    name      TEXT NOT NULL,
    parent_id INTEGER REFERENCES categories(id)
);

-- One column of each SQLite storage class, plus an untyped column.
CREATE TABLE measurements (
    id       INTEGER PRIMARY KEY,
    as_int   INTEGER,
    as_real  REAL,
    as_text  TEXT,
    as_blob  BLOB,
    as_null  NUMERIC,
    untyped,                     -- no declared type: takes any affinity
    taken_on DATE
);

-- Blobs and byte sizes.
CREATE TABLE documents (
    id         INTEGER PRIMARY KEY,
    title      TEXT NOT NULL,
    body       TEXT,
    content    BLOB,
    size_bytes INTEGER
);

-- A grab bag of less common declared types, to exercise type display.
CREATE TABLE mixed_types (
    id       INTEGER PRIMARY KEY,
    flag     BOOLEAN,
    amount   DECIMAL(10, 2),
    ratio    DOUBLE,
    big      BIGINT,
    label    VARCHAR(255),
    code     CHAR(8),
    payload  BLOB,
    happened TIMESTAMP,
    raw                          -- untyped
);

-- Simple key/value settings (text PK).
CREATE TABLE settings (
    key   TEXT PRIMARY KEY,
    value TEXT
);

-- Geo/JSON-ish events.
CREATE TABLE events (
    id      INTEGER PRIMARY KEY,
    name    TEXT NOT NULL,
    at      DATETIME,
    lat     REAL,
    lon     REAL,
    payload TEXT               -- JSON stored as text
);

-- Indexes.
CREATE INDEX idx_orders_user ON orders(user_id);
CREATE INDEX idx_order_items_product ON order_items(product_id);
CREATE INDEX idx_products_name ON products(name);
CREATE INDEX idx_events_at ON events(at);

-- View joining orders to their user and computed item count.
CREATE VIEW order_summary AS
SELECT o.id            AS order_id,
       u.name          AS customer,
       o.status        AS status,
       o.total         AS total,
       COUNT(oi.product_id) AS items
FROM orders o
JOIN users u ON u.id = o.user_id
LEFT JOIN order_items oi ON oi.order_id = o.id
GROUP BY o.id, u.name, o.status, o.total;

-- View of low-stock products.
CREATE VIEW low_stock AS
SELECT id, sku, name, in_stock
FROM products
WHERE in_stock < 5;

-- Trigger: stamp updated_at whenever a user row changes. Recursive
-- triggers are off by default, so this UPDATE does not re-fire.
CREATE TRIGGER users_set_updated
AFTER UPDATE ON users
FOR EACH ROW
BEGIN
    UPDATE users SET updated_at = datetime('now') WHERE id = OLD.id;
END;

-- ---------------------------------------------------------------------
-- Sample data
-- ---------------------------------------------------------------------

INSERT INTO users (name, email, age, is_active) VALUES
    ('Ada Lovelace',   'ada@example.com',    36, 1),
    ('Alan Turing',    'alan@example.com',   41, 1),
    ('Grace Hopper',   'grace@example.com',  85, 0),
    ('Edsger Dijkstra', NULL,                72, 1);

INSERT INTO products (id, sku, name, price, weight_kg, in_stock, description) VALUES
    (1, 'M855',    '5.56 M855 62gr',     0.62, 0.012, 240, 'Green tip'),
    (2, 'SMK175',  '.308 SMK 175gr',     1.35, 0.024,   3, 'Match king'),
    (3, 'BLEM',    'Blemished case',     8.00, 0.500,   0, NULL),
    (4, 'PRIMER',  'Large rifle primer', 0.08, 0.001, 999, 'Box of 100');

INSERT INTO categories (id, name, parent_id) VALUES
    (1, 'Ammunition', NULL),
    (2, 'Rifle',      1),
    (3, 'Components',  1),
    (4, 'Primers',     3);

INSERT INTO orders (id, user_id, total, status, ordered_at) VALUES
    (1, 1, 12.40, 'paid',    '2026-01-15 10:30:00'),
    (2, 2,  2.70, 'shipped', '2026-02-01 08:00:00'),
    (3, 1,  0.00, 'new',     '2026-08-17 09:00:00');

INSERT INTO order_items (order_id, product_id, quantity, unit_price) VALUES
    (1, 1, 20, 0.62),
    (1, 4,  1, 0.08),
    (2, 2,  2, 1.35);

INSERT INTO measurements (as_int, as_real, as_text, as_blob, as_null, untyped, taken_on) VALUES
    (42,   3.14159, 'hello',   x'deadbeef',           NULL, 7,       '2026-03-01'),
    (-7,   2.71828, 'world',   x'00ff10',             NULL, 'mixed', '2026-03-02'),
    (0,    0.0,     '',        x'',                    NULL, NULL,    NULL);

INSERT INTO documents (id, title, body, content, size_bytes) VALUES
    (1, 'Readme',    'Plain text body.',      NULL,          17),
    (2, 'Logo',      NULL,                    x'89504e470d0a1a0a', 8),
    (3, 'Empty',     '',                      x'',            0);

INSERT INTO mixed_types (flag, amount, ratio, big, label, code, payload, happened, raw) VALUES
    (1, 1234.56, 0.3333333333, 9223372036854775807, 'alpha', 'ABC12345', x'0102', '2026-04-01 12:00:00', 'anything'),
    (0,   -0.99, 1.5,          -42,                 'beta',  'X',        NULL,     NULL,                 3.14);

INSERT INTO settings (key, value) VALUES
    ('theme',        'dark'),
    ('page_size',    '50'),
    ('last_opened',  '2026-08-17'),
    ('experimental', NULL);

INSERT INTO events (id, name, at, lat, lon, payload) VALUES
    (1, 'login',    '2026-08-17 09:00:00', 50.4501, 30.5234, '{"ip":"10.0.0.1"}'),
    (2, 'purchase', '2026-08-17 09:05:00', NULL,    NULL,    '{"order":1}'),
    (3, 'logout',   '2026-08-17 09:30:00', 50.4501, 30.5234, NULL);
