# Distributed Password Cracker
Given a hash value, attempts to bruteforce the password in a timely manner by distributing the load across multiple worker nodes. 

Example usage:

./server <br />
./request_worker 127.0.0.1 5555 <br />
./request_client 127.0.0.1 5555 Kyq4bCxAXJkbg <br />