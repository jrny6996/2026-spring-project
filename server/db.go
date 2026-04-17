package main

import (
	"database/sql"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

var db *sql.DB

type User struct {
	ID           int64
	Username     string
	PasswordHash string
}

func initDB(path string) error {
	d, err := sql.Open("sqlite3", path+"?_foreign_keys=on")
	if err != nil {
		return err
	}
	d.SetMaxOpenConns(1)
	if err := d.Ping(); err != nil {
		return err
	}
	if _, err := d.Exec(`
CREATE TABLE IF NOT EXISTS users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT NOT NULL UNIQUE,
	password_hash TEXT NOT NULL,
	created_at TEXT NOT NULL
);`); err != nil {
		return err
	}
	db = d
	return nil
}

func getUserByUsername(username string) (*User, error) {
	row := db.QueryRow(`SELECT id, username, password_hash FROM users WHERE username = ?`, username)
	var u User
	if err := row.Scan(&u.ID, &u.Username, &u.PasswordHash); err != nil {
		return nil, err
	}
	return &u, nil
}

func createUser(username, passwordHash string) (int64, error) {
	res, err := db.Exec(
		`INSERT INTO users (username, password_hash, created_at) VALUES (?, ?, ?)`,
		username, passwordHash, time.Now().UTC().Format(time.RFC3339),
	)
	if err != nil {
		return 0, err
	}
	return res.LastInsertId()
}
