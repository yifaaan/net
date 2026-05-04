#!/usr/bin/env python3
import argparse
import asyncio
import struct
import time


async def worker(host: str, port: int, requests: int, payload: bytes) -> int:
    reader, writer = await asyncio.open_connection(host, port)
    ok = 0
    frame = struct.pack(">I", len(payload)) + payload
    try:
        for _ in range(requests):
            writer.write(frame)
            await writer.drain()
            header = await reader.readexactly(4)
            (n,) = struct.unpack(">I", header)
            data = await reader.readexactly(n)
            if data == payload:
                ok += 1
    finally:
        writer.close()
        await writer.wait_closed()
    return ok


async def run(host: str, port: int, clients: int, requests_per_client: int, payload_size: int) -> None:
    payload = b"x" * payload_size
    total = clients * requests_per_client
    start = time.perf_counter()
    tasks = [
        asyncio.create_task(worker(host, port, requests_per_client, payload))
        for _ in range(clients)
    ]
    done = await asyncio.gather(*tasks)
    elapsed = time.perf_counter() - start
    success = sum(done)
    qps = success / elapsed if elapsed > 0 else 0.0
    print(f"host={host} port={port}")
    print(f"clients={clients} requests_per_client={requests_per_client} payload_bytes={payload_size}")
    print(f"total_requests={total} success={success} elapsed_sec={elapsed:.4f}")
    print(f"qps={qps:.2f}")


def main() -> None:
    parser = argparse.ArgumentParser(description="QPS benchmark for net echo server.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=19090)
    parser.add_argument("--clients", type=int, default=64)
    parser.add_argument("--requests-per-client", type=int, default=2000)
    parser.add_argument("--payload-size", type=int, default=32)
    args = parser.parse_args()
    asyncio.run(
        run(
            host=args.host,
            port=args.port,
            clients=args.clients,
            requests_per_client=args.requests_per_client,
            payload_size=args.payload_size,
        )
    )


if __name__ == "__main__":
    main()
