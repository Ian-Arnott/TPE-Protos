SERVER="localhost"
PORT="1110"
USER="facha"
PASS="pass"

commands=$(cat << EOF
USER facha\nPASS pass\nRETR 1\nQUIT\n
EOF
)

echo "$commands" | telnet $SERVER $PORT > download.eml