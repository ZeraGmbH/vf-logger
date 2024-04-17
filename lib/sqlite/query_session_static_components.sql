SELECT
    sessions.session_name,
    entities.entity_name,
    components.component_name,
    valuemap.component_value
FROM sessions
INNER JOIN sessions_valuemap ON sessions.id = sessions_valuemap.sessionsid
INNER JOIN valuemap ON sessions_valuemap.valueid = valuemap.id
INNER JOIN components ON valuemap.componentid = components.id
INNER JOIN entities ON valuemap.entityiesid = entities.id
