CREATE TABLE components (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, component_name varchar(255));
CREATE TABLE entities (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, entity_name varchar(255));
CREATE TABLE records (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, record_name varchar(255) NOT NULL);
CREATE TABLE transactions (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, recordsid integer(10) NOT NULL, transaction_name varchar(255), contentset_name varchar(255), start_time timestamp, stop_time timestamp, FOREIGN KEY(recordsid) REFERENCES records(id));
CREATE TABLE transactions_valuemap (transactionsid integer(10) NOT NULL, valueid integer(10) NOT NULL, PRIMARY KEY (transactionsid, valueid), FOREIGN KEY(valueid) REFERENCES valuemap(id), FOREIGN KEY(transactionsid) REFERENCES transactions(id));
CREATE TABLE valuemap (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, value_timestamp timestamp, component_value numeric(19, 0), componentid integer(10), entityiesid integer(10), FOREIGN KEY(entityiesid) REFERENCES enties(id), FOREIGN KEY(componentid) REFERENCES component(id));

/*CREATE VIEW valueview AS SELECT records.record_name, transactions.transaction_name, enties.entity_name, component.component_name, valuemap.value_timestamp, valuemap.component_value FROM records INNER JOIN transactions ON records.id = transactions.recordsid INNER JOIN valuemap ON transactions.id = valuemap.transactionid INNER JOIN enties ON valuemap.* = enties.id INNER JOIN component ON valuemap.componentid = component.id; */

/* shows the data in a human readable / CSV processable form */;
/* example output:
      value_timestamp       | record_name |transaction_name |entity_name | component_name | component_value
----------------------------+-------------+-------------+----------------+--------------------
 2016-10-06 11:58:34.504319 |     default | someName        | someEntity |  someComponent | 2.3536
 2016-10-06 11:58:34.529059 |     default | someName        | someEntity |  someComponent | some string
 2016-10-06 11:58:34.553821 |     default | someName        | someEntity |  someComponent | {9.2453,36.436347}
*/;

