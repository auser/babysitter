{application, babysitter,
 [
  {description, "Babysitter app"},
  {vsn, "0.1"},
  {id, "babysitter"},
  {modules,      [
      babysitter, babysitter_app, babysitter_config,
      babysitter_config_parser, babysitter_list_utils, babysitter_port, babysitter_sup, make_boot,
      os_process, reloader, string_utils
    ]},
  {registered,   []},
  {applications, [kernel, stdlib]},
  {mod, {babysitter_app, []}},
  {env, [
    {debug, 3}
  ]}
 ]
}.