/* The record is a series of values collected over a variable duration connected to customer data not included in this schema */;
CREATE TABLE records (record_id INTEGER PRIMARY KEY, record_name VARCHAR(255) NOT NULL) WITHOUT ROWID;

/* SDK based entity id */;
CREATE TABLE entities (entity_id INTEGER PRIMARY KEY, entity_name VARCHAR(255) NOT NULL) WITHOUT ROWID;

/* SDK based component name, the id is not relevant to the SDK,
currently the system features around 200 components with variable update intervals ranging from "once then never" to a maximal theoretical of 10Hz */;
CREATE TABLE components (component_id INTEGER PRIMARY KEY, component_name VARCHAR(255) UNIQUE) WITHOUT ROWID;

CREATE TABLE valuemap (valuemap_id INTEGER PRIMARY KEY,
                       entity_id INTEGER REFERENCES entities(entity_id) NOT NULL,
                       component_id INTEGER REFERENCES components(component_id) NOT NULL,
                       value_timestamp VARCHAR(255) NOT NULL,
                       component_value NUMERIC) WITHOUT ROWID; /* can be any type */

/* One value can be logged to multiple records simultaneously */;
CREATE TABLE recordmapping (record_id int REFERENCES records(id) NOT NULL,
                            valuemap_id int REFERENCES valuemapping(id) NOT NULL);
