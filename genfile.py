import ecdsa
import os
import hashlib
import base58
from Crypto.Hash import RIPEMD160

def generate_btc_address_from_int(start, end):
    """Generate BTC addresses for a given range and write them to a file."""
    output_file = f"{start}-{end-1}.txt"  # File name follows "start-end-1.txt"

    with open(output_file, "a") as file:
        for num in range(start, end):
            private_key = num.to_bytes(32, 'big')
            sk = ecdsa.SigningKey.from_string(private_key, curve=ecdsa.SECP256k1)
            vk = sk.verifying_key
            public_key = b"\x04" + vk.to_string()
            
            sha256_pk = hashlib.new("sha256", public_key).digest()
            ripemd160 = RIPEMD160.new(sha256_pk).digest()
            network_byte = b"\x00" + ripemd160
            checksum = hashlib.new("sha256", hashlib.new("sha256", network_byte).digest()).digest()[:4]
            
            address_bytes = network_byte + checksum
            bitcoin_address = base58.b58encode(address_bytes).decode()
            
            file.write(f"{bitcoin_address}\n")  # Only write the BTC address

if __name__ == "__main__":
    start = 1
    end = 10 
    generate_btc_address_from_int(start, end)
    with open("status.txt", "a") as file:
            file.write(f"{end - 1}")  
    os.system("git add status.txt")
    os.system("git commit -m auto")
    os.system("git push")
        


