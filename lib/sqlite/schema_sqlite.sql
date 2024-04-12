CREATE TABLE components (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    component_name varchar(255)
    );

CREATE TABLE entities (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    entity_name varchar(255)
    );

CREATE TABLE sessions (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    session_name varchar(255) NOT NULL);

CREATE TABLE transactions (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    sessionid integer(10) NOT NULL,
    transaction_name varchar(255),
    contentset_names varchar(255),
    guicontext_name varchar(255),
    start_time timestamp,
    stop_time timestamp,
    FOREIGN KEY (sessionid) REFERENCES sessions(id)
    );

CREATE TABLE transactions_valuemap (
    transactionsid integer(10) NOT NULL,
    valueid integer(10) NOT NULL,
    PRIMARY KEY (transactionsid, valueid),
    FOREIGN KEY (valueid) REFERENCES valuemap(id),
    FOREIGN KEY (transactionsid) REFERENCES transactions(id)
    );

CREATE TABLE sessions_valuemap (
    sessionsid integer(10) NOT NULL,
    valueid integer(10) NOT NULL,
    PRIMARY KEY (sessionsid, valueid),
    FOREIGN KEY (valueid) REFERENCES valuemap(id),
    FOREIGN KEY (sessionsid) REFERENCES sessions(id));

CREATE TABLE valuemap (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    value_timestamp timestamp,
    component_value numeric(19, 0),
    componentid integer(10),
    entityiesid integer(10),
    FOREIGN KEY (entityiesid) REFERENCES entities(id),
    FOREIGN KEY (componentid) REFERENCES components(id)
    );

