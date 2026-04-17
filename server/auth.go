package main

import (
	"database/sql"
	"encoding/json"
	"errors"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/golang-jwt/jwt/v4"
	"golang.org/x/crypto/bcrypt"
)

type jwtClaims struct {
	UserID   int64  `json:"uid"`
	Username string `json:"username"`
	jwt.RegisteredClaims
}

var jwtSecret []byte

func init() {
	s := os.Getenv("JWT_SECRET")
	if len(s) < 32 {
		jwtSecret = []byte("dev-secret-change-in-production-min-32-chars!!")
	} else {
		jwtSecret = []byte(s)
	}
}

func hashPassword(pw string) ([]byte, error) {
	return bcrypt.GenerateFromPassword([]byte(pw), bcrypt.DefaultCost)
}

func signToken(userID int64, username string) (string, error) {
	claims := jwtClaims{
		UserID:   userID,
		Username: username,
		RegisteredClaims: jwt.RegisteredClaims{
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(24 * time.Hour)),
			IssuedAt:  jwt.NewNumericDate(time.Now()),
		},
	}
	t := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	return t.SignedString(jwtSecret)
}

func parseTokenString(s string) (*jwtClaims, error) {
	var claims jwtClaims
	_, err := jwt.ParseWithClaims(s, &claims, func(t *jwt.Token) (interface{}, error) {
		return jwtSecret, nil
	})
	if err != nil {
		return nil, err
	}
	return &claims, nil
}

func parseTokenFromRequest(r *http.Request) (*jwtClaims, error) {
	tok := r.URL.Query().Get("token")
	if tok == "" {
		h := r.Header.Get("Authorization")
		const p = "bearer "
		if len(h) > len(p) && strings.EqualFold(h[:len(p)], p) {
			tok = strings.TrimSpace(h[len(p):])
		}
	}
	if tok == "" {
		return nil, errors.New("missing token")
	}
	return parseTokenString(tok)
}

type credsBody struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

func validateUsername(s string) error {
	if len(s) < 3 || len(s) > 32 {
		return errors.New("username must be 3–32 characters")
	}
	for _, c := range s {
		ok := (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'
		if !ok {
			return errors.New("username may only use letters, digits, underscore")
		}
	}
	return nil
}

func respondJSON(w http.ResponseWriter, status int, v interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(v)
}

func registerHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var body credsBody
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		respondJSON(w, http.StatusBadRequest, map[string]string{"error": "invalid json"})
		return
	}
	if err := validateUsername(body.Username); err != nil {
		respondJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	if len(body.Password) < 8 {
		respondJSON(w, http.StatusBadRequest, map[string]string{"error": "password must be at least 8 characters"})
		return
	}
	if _, err := getUserByUsername(body.Username); err == nil {
		respondJSON(w, http.StatusConflict, map[string]string{"error": "username taken"})
		return
	} else if !errors.Is(err, sql.ErrNoRows) {
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "database error"})
		return
	}
	hash, err := hashPassword(body.Password)
	if err != nil {
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not hash password"})
		return
	}
	id, err := createUser(body.Username, string(hash))
	if err != nil {
		if strings.Contains(err.Error(), "UNIQUE constraint failed") {
			respondJSON(w, http.StatusConflict, map[string]string{"error": "username taken"})
			return
		}
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not create user"})
		return
	}
	token, err := signToken(id, body.Username)
	if err != nil {
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not issue token"})
		return
	}
	respondJSON(w, http.StatusCreated, map[string]interface{}{
		"token":    token,
		"username": body.Username,
		"id":       id,
	})
}

func loginHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var body credsBody
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		respondJSON(w, http.StatusBadRequest, map[string]string{"error": "invalid json"})
		return
	}
	u, err := getUserByUsername(body.Username)
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			respondJSON(w, http.StatusUnauthorized, map[string]string{"error": "invalid credentials"})
			return
		}
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "database error"})
		return
	}
	if bcrypt.CompareHashAndPassword([]byte(u.PasswordHash), []byte(body.Password)) != nil {
		respondJSON(w, http.StatusUnauthorized, map[string]string{"error": "invalid credentials"})
		return
	}
	token, err := signToken(u.ID, u.Username)
	if err != nil {
		respondJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not issue token"})
		return
	}
	respondJSON(w, http.StatusOK, map[string]interface{}{
		"token":    token,
		"username": u.Username,
		"id":       u.ID,
	})
}

func meHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	claims, err := parseTokenFromRequest(r)
	if err != nil {
		respondJSON(w, http.StatusUnauthorized, map[string]string{"error": "unauthorized"})
		return
	}
	respondJSON(w, http.StatusOK, map[string]interface{}{
		"id":       claims.UserID,
		"username": claims.Username,
	})
}
