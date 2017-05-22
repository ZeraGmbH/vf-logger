/* Selects all stored values */;
/* example output:
 valuemap_id | record_name | entity_name | component_name |      value_timestamp       | component_value
-------------+-------------+-------------+----------------+----------------------------+--------------------
           1 |     record1 |  someEntity |  someComponent | 2016-10-06 11:58:34.504319 | 2.3536
           2 |     record1 |  someEntity |  someComponent | 2016-10-06 11:58:34.529059 | some string
           3 |     record1 |  someEntity |  someComponent | 2016-10-06 11:58:34.553821 | {9.2453,36.436347}
*/;

SELECT valuemap_id, record_name, entity_name, component_name, value_timestamp, component_value FROM valuemap
  NATURAL JOIN recordmapping
  NATURAL JOIN records
  NATURAL JOIN entities
  NATURAL JOIN components
  ORDER BY 1;
