/* 
 * This file will generate all the tables required on a database.
 * The database is mostly like the official Teamspeak one, except we
 * try to use humanly readable fields
 */

CREATE TABLE servers (
  id integer primary key auto_increment,
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
  id integer primary key auto_increment,
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
  id integer primary key auto_increment,
  server_id integer,
  serveradmin integer,
  name varchar(40),
  password varchar(80),
  created_at varchar(20),
  lastonline varchar(20)
);
