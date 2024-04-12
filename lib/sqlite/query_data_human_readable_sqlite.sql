/* shows the data in a human readable / CSV processable form */;
/* example output:
      value_timestamp       | session_name |transaction_name |entity_name | component_name | component_value
----------------------------+--------------+-------------+----------------+--------------------
 2016-10-06 11:58:34.504319 |     default  | someName        | someEntity |  someComponent | 2.3536
 2016-10-06 11:58:34.529059 |     default  | someName        | someEntity |  someComponent | some string
 2016-10-06 11:58:34.553821 |     default  | someName        | someEntity |  someComponent | {9.2453,36.436347}
*/;

SELECT
        sessions.session_name,
        transactions.transaction_name,
        entities.entity_name,
        components.component_name,
        valuemap.value_timestamp,
        valuemap.component_value
FROM sessions
INNER JOIN transactions ON sessions.id = transactions.sessionid
INNER JOIN transactions_valuemap ON transactions.id = transactions_valuemap.transactionsid
INNER JOIN valuemap ON valuemap.id = transactions_valuemap.valueid
INNER JOIN entities ON valuemap.entityiesid = entities.id
INNER JOIN components ON valuemap.componentid = components.id
ORDER BY valuemap.value_timestamp

