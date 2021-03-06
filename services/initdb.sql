-- under OS user postgres, execute the following command
-- create a super user for this demo
[postgres@xxxxxx ~]$ createuser --interactive

-- create the demo database
[postgres@xxxxxx ~]$ createdb demo

-- connect demo database
psql demo
or
psql, the \c demo

-- create SCHEMA
CREATE SCHEMA identity;
-- set search_path
SET search_path=identity;

-- create table users (for demo purpose)
CREATE TABLE users(
  staff_id SERIAL PRIMARY KEY,
  first_name VARCHAR(16) NOT NULL,
  last_name VARCHAR(16) NOT NULL,
  email VARCHAR(64) NOT NULL UNIQUE,
  age INTEGER
);

-- some data to make table users dirty
INSERT INTO users VALUES(DEFAULT, 'Bill', 'Gates', 'bg@microsoft.com', 62);
INSERT INTO users VALUES(DEFAULT, 'Michael', 'Jackson', 'mj@google.com', 68);
INSERT INTO users VALUES(DEFAULT, 'Marry', 'Popins', 'mp@magicworld.com', 208);
INSERT INTO users VALUES(DEFAULT, 'Tom', 'Bear', 'tb@magicworld.com', 198);
INSERT INTO users VALUES(DEFAULT, 'Larry', 'King', 'lk@cnn.com', 75);
INSERT INTO users VALUES(DEFAULT, 'Donald', 'Trump', 'dt@usa.com', 75);
INSERT INTO users VALUES(DEFAULT, 'Bill', 'Clinton', 'bc@usa.com', 73);
