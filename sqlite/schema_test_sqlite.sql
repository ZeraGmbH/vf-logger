/* enable foreign key checking in sqlite */;
PRAGMA foreign_keys = ON;
/* The record is a series of values collected over a variable duration connected to customer data not included in this schema */;
CREATE TABLE records (record_id INTEGER PRIMARY KEY, record_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;
/* SDK based entity id */
CREATE TABLE entities (entity_id INTEGER PRIMARY KEY, entity_name VARCHAR(255) NOT NULL) WITHOUT ROWID;
/* SDK based component name, the id is not relevant world outside of the database and it is assigned automagical by the DB */;
CREATE TABLE components (component_id INTEGER PRIMARY KEY, component_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;
CREATE TABLE valuemap (valuemap_id INTEGER PRIMARY KEY,
                       entity_id INTEGER REFERENCES entities(entity_id) NOT NULL,
                       component_id INTEGER REFERENCES components(component_id) NOT NULL,
                       value_timestamp VARCHAR(255) NOT NULL, /* timestamp in ISO 8601 format, example: 2016-10-06 11:58:34.504319 */
                       component_value NUMERIC) WITHOUT ROWID; /* can be any type but numeric is preferred */
/* One value can be logged to multiple records simultaneously */
CREATE TABLE recordmapping (record_id INTEGER REFERENCES records(record_id) NOT NULL,
                            valuemap_id INTEGER REFERENCES valuemap(valuemap_id) NOT NULL);
/* shows the data in a human readable / CSV processable form */;
/* example output:
      value_timestamp       | record_name | entity_name | component_name | component_value
----------------------------+-------------+-------------+----------------+--------------------
 2016-10-06 11:58:34.504319 |     record1 |  someEntity |  someComponent | 2.3536
 2016-10-06 11:58:34.529059 |     record1 |  someEntity |  someComponent | some string
 2016-10-06 11:58:34.553821 |     record1 |  someEntity |  someComponent | {9.2453,36.436347}
*/;
CREATE VIEW valueview AS SELECT value_timestamp, record_name, entity_name, component_name, component_value FROM valuemap
  NATURAL JOIN recordmapping
  NATURAL JOIN records
  NATURAL JOIN entities
  NATURAL JOIN components
  ORDER BY 1;
