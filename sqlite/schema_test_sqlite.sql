/* enable foreign key checking in sqlite */
PRAGMA foreign_keys = ON;

/* The record is a series of values collected over a variable duration connected to customer data not included in this schema */
CREATE TABLE records (record_id INTEGER PRIMARY KEY, record_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;

/* SDK based entity id */
CREATE TABLE entities (entity_id INTEGER PRIMARY KEY, entity_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;

/* SDK based component name, the id is not relevant to the SDK,
currently the system features around 200 components with variable update intervals ranging from "once then never" to a theoretical maximum of 10Hz */
CREATE TABLE components (component_id INTEGER PRIMARY KEY, component_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;

CREATE TABLE valuemap (valuemap_id INTEGER PRIMARY KEY,
                       entity_id INTEGER REFERENCES entities(entity_id) NOT NULL,
                       component_id INTEGER REFERENCES components(component_id) NOT NULL,
                       value_timestamp VARCHAR(255) NOT NULL, /* timestamp in ISO 8601 format, example: 2016-10-06 11:58:34.504319 */
                       component_value NUMERIC) WITHOUT ROWID; /* can be any type but numeric is preferred */

/* One value can be logged to multiple records simultaneously */
CREATE TABLE recordmapping (record_id INTEGER REFERENCES records(record_id) NOT NULL,
                            valuemap_id INTEGER REFERENCES valuemap(valuemap_id) NOT NULL);
