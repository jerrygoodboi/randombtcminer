import ecdsa
import time
import hashlib
import base58
import ssl
import socket
import json
import multiprocessing
from Crypto.Hash import RIPEMD160
from queue import Queue

# Server and output file
SERVER = "164.92.148.39:50002"
OUTPUT_FILE = "300000000.txt"

def addr_to_scripthash(addr):
    """Convert Bitcoin address to ElectrumX scripthash."""
    return hashlib.new("sha256", b'\x76\xa9\x14' + base58.b58decode_check(addr)[1:] + b'\x88\xac').digest()[::-1].hex()

def get_balance(addr):
    """Get balance for a single Bitcoin address from ElectrumX."""
    host, port = SERVER.split(":")
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE

    try:
        with socket.create_connection((host, int(port)), timeout=5) as s:
            with context.wrap_socket(s, server_hostname=host) as ss:
                msg = json.dumps({
                    "id": 0, "method": "blockchain.scripthash.get_balance",
                    "params": [addr_to_scripthash(addr)]
                }).encode() + b"\n"

                ss.sendall(msg)
                response = json.loads(ss.recv(8192).decode())  # Increased buffer size
                print(response)

                return int(response.get("result", {}).get("confirmed", 0))  # Avoid KeyError

    except (socket.timeout, json.JSONDecodeError, ConnectionError) as e:
        print(f"Error fetching balance: {e}")
        return 0  # Assume 0 balance on failure

def generate_keys_and_check_balance(start, end, queue):
    """Generate private keys, get BTC addresses, and check balances."""
    stime = time.time()

    for num in range(start, end):
        curr = time.time()
        if curr - stime > 30:
            print(f"Checked up to: {num}")
            stime = curr
        num = 115792089237316195423570985008687907852837564279074904382605163141518161494336 
        private_key = num.to_bytes(32, 'big')

        # Generate public key
        sk = ecdsa.SigningKey.from_string(private_key, curve=ecdsa.SECP256k1)
        vk = sk.verifying_key
        public_key = b"\x04" + vk.to_string()

        # Hashing (optimized)
        sha256_pk = hashlib.new("sha256", public_key).digest()
        ripemd160 = RIPEMD160.new(sha256_pk).digest()

        # Create Bitcoin address
        network_byte = b"\x00" + ripemd160
        checksum = hashlib.new("sha256", hashlib.new("sha256", network_byte).digest()).digest()[:4]
        bitcoin_address = base58.b58encode(network_byte + checksum).decode()

        balance = get_balance(bitcoin_address)

        if balance > 0:
            queue.put(f"{private_key.hex()} {bitcoin_address} {balance}\n")

def file_writer(queue):
    """Writes results to file efficiently."""
    with open(OUTPUT_FILE, "a") as f:
        while True:
            data = queue.get()
            if data == "STOP":
                break
            f.write(data)

def main():
    start = 300000000
    end = 600000000
    num_workers = multiprocessing.cpu_count()

    chunk_size = (end - start) // num_workers
    processes = []
    queue = multiprocessing.Queue()

    # Start file writer process
    writer_process = multiprocessing.Process(target=file_writer, args=(queue,))
    writer_process.start()

    # Start worker processes
    for i in range(num_workers):
        p_start = start + i * chunk_size
        p_end = start + (i + 1) * chunk_size
        p = multiprocessing.Process(target=generate_keys_and_check_balance, args=(p_start, p_end, queue))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()

    queue.put("STOP")
    writer_process.join()

if __name__ == "__main__":
    main()

