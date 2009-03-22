/* 
 * This file will generate all the tables required on a database.
 * The database is mostly like the official Teamspeak one, except we
 * try to use humanly readable fields
 */

CREATE TABLE servers (
  id integer primary key autoincrement,
  name varchar(40),
  welcome_msg varchar(80) ,
  maxusers integer ,
  port integer ,
  password varchar(80) ,
  clan_server integer,
  codec_celp51 integer ,
  codec_celp63 integer ,
  codec_gsm148 integer ,
  codec_gsm164 integer ,
  codec_celp52 integer ,
  codec_speex2150 integer ,
  codec_speex3950 integer ,
  codec_speex5950 integer ,
  codec_speex8000 integer ,
  codec_speex11000 integer ,
  codec_speex15000 integer ,
  codec_speex18200 integer ,
  codec_speex24600 integer ,
  webposturl varchar(200) ,
  weblinkurl varchar(200) ,
  active integer,
  created_at varchar(20),
  description varchar(100)
);

CREATE TABLE channels (
  id integer primary key autoincrement,
  server_id integer ,
  parent_id integer ,
  flag_moderated integer,
  flag_hierarchical integer,
  flag_default integer,
  codec integer ,
  ordr integer ,
  maxusers integer ,
  name varchar(40) ,
  topic varchar(40) ,
  description text,
  password varchar(80),
  created_at varchar(20)
);

CREATE TABLE registrations (
  id integer primary key autoincrement,
  server_id integer,
  serveradmin integer,
  name varchar(40),
  password varchar(80),
  created_at varchar(20),
  lastonline varchar(20)
);

CREATE TABLE server_privileges (
  id integer primary key autoincrement,
  server_id integer,
  user_group varchar(40), /* SA, CA, Voice, OP, Anonymous */
  /* Administration privileges */
  adm_del_server integer,
  adm_add_server integer,
  adm_list_servers integer,
  adm_set_permissions integer,
  adm_change_user_pass integer,
  adm_change_own_pass integer,
  adm_list_registrations integer,
  adm_register_player integer,
  adm_change_server_codecs integer,
  adm_change_server_type integer,
  adm_change_server_pass integer,
  adm_change_server_welcome integer,
  adm_change_server_maxusers integer,
  adm_change_server_name integer,
  adm_change_webpost_url integer,
  adm_change_server_port integer,
  adm_start_server integer,
  adm_stop_server integer,
  adm_move_player integer,
  adm_ban_ip integer,
  /* Channel privileges */
  cha_delete integer,
  cha_create_moderated integer,
  cha_create_subchanneled integer,
  cha_create_default integer,
  cha_create_unregistered integer,
  cha_create_registered integer,
  cha_join_registered integer,
  cha_join_wo_pass integer,
  cha_change_codec integer,
  cha_change_maxusers integer,
  cha_change_order integer,
  cha_change_desc integer,
  cha_change_topic integer,
  cha_change_pass integer,
  cha_change_name integer,
  /* Player privileges */
  pl_grant_allowreg integer,
  pl_grant_voice integer,
  pl_grant_autovoice integer,
  pl_grant_op integer,
  pl_grant_autoop integer,
  pl_grant_ca integer,
  pl_grant_sa integer,
  pl_register_player integer,
  pl_revoke_allowreg integer,
  pl_revoke_voice integer,
  pl_revoke_autovoice integer,
  pl_revoke_op integer,
  pl_revoke_autoop integer,
  pl_revoke_ca integer,
  pl_revoke_sa integer,
  pl_allow_self_reg integer,
  pl_del_registration integer,
  /* Other privileges */
  other_ch_commander integer,
  other_ch_kick integer,
  other_sv_kick integer,
  other_text_pl integer,
  other_text_all_ch integer,
  other_text_in_ch integer,
  other_text_all integer
);

CREATE TABLE player_channel_privileges (
  id integer primary key autoincrement,
  player_id integer,
  channel_id integer,
  /* privileges */
  channel_admin integer,
  operator integer,
  voice integer,
  auto_operator integer,
  auto_voice integer
);
