import ecdsa
import os
import hashlib
import base58
import multiprocessing
from Crypto.Hash import RIPEMD160

FOUND_FILE = "found_btc_addresses.txt"
FILE_LOCK = multiprocessing.Lock()  # Use multiprocessing lock
PROCESS_COUNT = multiprocessing.cpu_count()  # Use all CPU cores

# Load sorted file into memory (fast lookup with a set)
def load_addresses(filename):
    with open(filename, "r") as f:
        return set(line.strip() for line in f)  # Strip newlines and store in a set

# Generate Bitcoin address from public key
def generate_btc_address_from_pubkey(public_key):
    sha256_pk = hashlib.sha256(public_key).digest()
    ripemd160 = RIPEMD160.new(sha256_pk).digest()
    network_byte = b"\x00" + ripemd160
    checksum = hashlib.sha256(hashlib.sha256(network_byte).digest()).digest()[:4]
    address_bytes = network_byte + checksum
    return base58.b58encode(address_bytes).decode()

# Infinite loop for generating and searching
def generate_and_search(addresses_set):
    while True:  # Infinite loop
        private_key = os.urandom(32)
        sk = ecdsa.SigningKey.from_string(private_key, curve=ecdsa.SECP256k1)
        vk = sk.verifying_key
        public_key = b"\x04" + vk.to_string()

        btc_address = generate_btc_address_from_pubkey(public_key)

        if btc_address in addresses_set:  # Ultra-fast set lookup (O(1))
            private_key_hex = private_key.hex()
            with FILE_LOCK:  # Lock only for writing
                with open(FOUND_FILE, "a") as f_out:
                    f_out.write(f"{private_key_hex} {btc_address}\n")

# Multi-processing execution
def multiprocess_btc_search(filename):
    addresses_set = load_addresses(filename)  # Load file into memory once
    processes = []
    for _ in range(PROCESS_COUNT):  # Create as many processes as CPU cores
        p = multiprocessing.Process(target=generate_and_search, args=(addresses_set,))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()  # Keep processes running indefinitely

# Run the search
if __name__ == "__main__":
    multiprocess_btc_search("sortlegacy.txt")

