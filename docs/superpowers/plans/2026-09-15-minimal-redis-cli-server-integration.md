# Minimal Redis CLI/Server Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the minimal RESP2 Redis server behavior required for the upstream C++ CLI to execute PING, SET, GET, DEL, and EXISTS over TCP.

**Architecture:** Keep the server and upstream CLI as separate repositories. Add a bounded RESP2 frame parser, a synchronized in-memory key/value store, and command execution to the server; repair the per-client TCP loop so it handles partial frames, multiple requests, EOF, and short writes.

**Tech Stack:** C++17, POSIX sockets, `std::thread`, `std::mutex`, `std::unordered_map`, Make, shell-based integration tests.

---

### Task 1: Add deterministic RESP2 parsing and serialization tests

**Files:**
- Create: `tests/RedisProtocolTests.cpp`
- Modify: `Makefile`

- [ ] **Step 1: Write tests for complete and incomplete requests**

Test `PING`, `SET` containing spaces, two concatenated requests, incomplete bulk payloads, and malformed prefixes. The parser API under test is `RespParser::tryParseCommand(buffer, command, consumed)`.

- [ ] **Step 2: Write tests for response strings**

Assert exact bytes for `+PONG\r\n`, `+OK\r\n`, `:1\r\n`, `$5\r\nhello\r\n`, `$-1\r\n`, and `-ERR wrong number of arguments\r\n`.

- [ ] **Step 3: Add a `test` Make target**

Compile protocol tests with the protocol implementation and run `./build/redis_protocol_tests`; keep the existing `all`, `clean`, and `run` targets working.

- [ ] **Step 4: Run the new tests and confirm they fail for missing symbols**

Run `make clean && make test`. Expected result: compilation fails because `RespParser` has not been implemented.

### Task 2: Implement RESP2 request parsing and response serialization

**Files:**
- Create: `include/RespParser.h`
- Create: `src/RespParser.cpp`
- Modify: `Makefile`

- [ ] **Step 1: Define the parser interface**

Use `enum class ParseResult { Complete, Incomplete, Invalid }`, `std::vector<std::string>` commands, and a `tryParseCommand` method that reports the number of consumed bytes only for complete frames.

- [ ] **Step 2: Implement bounded RESP2 parsing**

Require an array prefix, decimal non-negative element count, bulk-string prefixes, exact lengths, and CRLF delimiters. Return `Incomplete` when more bytes are needed and `Invalid` for malformed input. Never use `stoi` without validating the field or index past the buffer.

- [ ] **Step 3: Implement response helpers**

Add static helpers for simple strings, errors, integers, bulk strings, and null bulk strings. Compute lengths from bytes and always use `\r\n`.

- [ ] **Step 4: Run protocol tests**

Run `make test`. Expected result: all parser and serializer tests pass.

### Task 3: Implement the shared in-memory database and command behavior

**Files:**
- Create: `include/RedisDatabase.h`
- Create: `src/RedisDatabase.cpp`
- Modify: `include/RedisCommandHandler.h`
- Modify: `src/RedisCommandHandler.cpp`
- Modify: `tests/RedisProtocolTests.cpp`

- [ ] **Step 1: Add command behavior tests**

Construct one `RedisCommandHandler`, execute `PING`, `SET key value`, `GET key`, `EXISTS key`, `DEL key`, and missing-key `GET`; assert exact RESP2 responses. Also assert unsupported commands and wrong arity return errors.

- [ ] **Step 2: Implement `RedisDatabase`**

Store `std::unordered_map<std::string, std::string>` behind one `std::mutex`. Expose `set`, `get`, `del`, and `exists`; return `std::optional<std::string>` for missing values and `bool`/count results for mutations.

- [ ] **Step 3: Implement `RedisCommandHandler`**

Accept a shared database in the constructor, uppercase only the command name, validate arity, execute the five commands, and serialize every result through `RespParser` helpers. Do not print command arguments from server request handling.

- [ ] **Step 4: Run unit tests**

Run `make test`. Expected result: parser and command tests pass.

### Task 4: Repair the TCP server loop for the CLI contract

**Files:**
- Modify: `include/RedisServer.h`
- Modify: `src/RedisServer.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: Add a per-client handler API and shared handler state**

Create one database and command handler for the server, pass the handler by reference to each client thread, and add private `handleClient` and `sendAll` helpers.

- [ ] **Step 2: Implement buffered request processing**

Append `recv` bytes to a per-client string, repeatedly parse complete requests, execute them, erase consumed bytes, and retain incomplete bytes for the next read. Reject invalid frames with an error response and close that client connection.

- [ ] **Step 3: Handle socket lifecycle correctly**

Treat `recv == 0` as normal EOF, handle `EINTR`, avoid constructing strings from negative byte counts, close client sockets on every exit path, and join worker threads during shutdown. Use `sendAll` to handle short writes and `SIGPIPE` safely.

- [ ] **Step 4: Make shutdown and startup predictable**

Validate the port argument in `main`, install a signal handler that calls `shutdown`, and remove the detached infinite persistence thread because persistence is out of scope.

- [ ] **Step 5: Build the server**

Run `make clean && make`. Expected result: `my_redis_server` builds with no warnings treated as errors and `make test` remains green.

### Task 5: Build the upstream CLI and run end-to-end verification

**Files:**
- Create outside the server repository: `/Users/SIPSS0673/Desktop/projects/Redis-CLI` by cloning `https://github.com/Cukowski/Redis-CLI.git`
- No source changes to the upstream client unless a tested compatibility defect prevents the requested workflow.

- [ ] **Step 1: Clone and build the GitHub client**

Run `git clone https://github.com/Cukowski/Redis-CLI.git /Users/SIPSS0673/Desktop/projects/Redis-CLI`, then `make -C '/Users/SIPSS0673/Desktop/projects/Redis-CLI/Part 6'`.

- [ ] **Step 2: Start the server on an unused test port**

Run `./my_redis_server 6389` from the server worktree and capture its PID without using the default Redis port.

- [ ] **Step 3: Execute one-shot client commands**

Run the Part 6 client with `-h 127.0.0.1 -p 6389` for `PING`, `SET name Alice`, `GET name`, `GET missing`, `EXISTS name`, `DEL name`, and `EXISTS name`; verify successful exit and expected output.

- [ ] **Step 4: Exercise one persistent CLI connection**

Pipe `PING`, `SET greeting hello`, `GET greeting`, and `quit` into the interactive client and verify all responses arrive on one TCP connection.

- [ ] **Step 5: Run final checks**

Run `make test`, `make clean && make`, and `git diff --check`; record the exact commands and results in the final handoff.

