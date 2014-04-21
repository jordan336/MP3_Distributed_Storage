MP3_Distributed_Storage
=======================

ECE 428 - Distributed Systems
Distributed Storage MP

- Mark Kennedy - kenned31
- Jordan Ebel  - ebel1


Usage
=====
./store config_file id


Config File
===========
format: 
    num_processes \n 
    ip_address_1  delay_to_1 \n 
    ip_address_2  delay_to_2 \n 
    ... 


To Do
=====
- Use ACKs to enforce consistency level
- Message delays
- Inconsistency repair


Change Log
==========

- 4/15: project set up
- 4/16: read in user commands
- 4/17: message passing interface working
- 4/20: operations working
- 4/20: owner contacting replicas to do operation and ACK transmitting

