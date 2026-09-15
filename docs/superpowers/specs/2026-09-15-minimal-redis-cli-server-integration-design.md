# Minimal Redis CLI/Server Integration Design

## Goal

Make the current C++ Redis server interoperable with the upstream C++ CLI in `Cukowski/Redis-CLI`, using the CLI's existing RESP2-over-TCP contract.

## Scope

The GitHub client remains a separate repository and is used as the client contract. The server implements the minimal command set needed for a useful CLI smoke test:

- `PING`
- `SET key value`
- `GET key`
- `DEL key`
- `EXISTS key`

The CLI-local `help`, `quit`, and `exit` behaviors remain client responsibilities. Persistence, authentication, expiration, lists, hashes, sets, pub/sub, transactions, and Redis command introspection are explicitly out of scope.

## Architecture and data flow

The server keeps its existing TCP listener and per-connection threading model, with these focused responsibilities:

1. The listener accepts clients and starts a handler for each socket.
2. The connection handler accumulates bytes because TCP reads may split or combine RESP requests.
3. A RESP2 request parser extracts complete array-of-bulk-string commands and leaves incomplete bytes buffered.
4. A command handler validates arity and executes commands against a shared, mutex-protected in-memory key/value store.
5. Responses are serialized as valid RESP2 simple strings, errors, integers, and bulk strings.

The store is process-local and is shared by all client connections. Every command receives one response before the handler reads the next command. Client EOF closes only that client connection; server shutdown closes the listening socket.

## Protocol contract

The GitHub CLI serializes a command such as `SET greeting hello` as:

```text
*3\r\n$3\r\nSET\r\n$8\r\ngreeting\r\n$5\r\nhello\r\n
```

The server returns:

```text
PING              +PONG\r\n
SET key value     +OK\r\n
GET existing      $<length>\r\n<value>\r\n
GET missing       $-1\r\n
DEL existing      :1\r\n
DEL missing       :0\r\n
EXISTS existing   :1\r\n
EXISTS missing    :0\r\n
```

Malformed requests, unsupported commands, and wrong arity return Redis-style `-ERR ...\r\n` responses without terminating the connection. Socket sends must handle short writes.

## Error handling and concurrency

The parser rejects invalid prefixes, lengths, missing CRLF delimiters, and incomplete frames without reading past the buffer. The connection handler treats `recv == 0` as normal client disconnect and handles socket errors without spinning. Store access is locked for each command, so concurrent clients cannot corrupt the map.

## Validation

Validation will include unit tests for RESP parsing and command behavior, a clean server build, a clean build of the GitHub client's latest `Part 6` client, and an end-to-end run that executes `PING`, `SET`, `GET`, `DEL`, and missing-key behavior through the CLI against the server.

