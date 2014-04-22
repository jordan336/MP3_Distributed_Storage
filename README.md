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

Command Execution
==================
- Process where command was typed :  parse input -> contact owner -> wait for owner to give result -> return
- Owner process with level > 1    :  receive message -> create pthread -> do command -> contact replicas -> wait for all replicas to ack -> reply to original process
- Owner process with level <= 1   :  receive message -> create pthread -> do command -> contact replicas -> reply to original process and fork a process to handle replica acks in background
- Replica process with any level  :  receive message -> create pthread -> do command -> send ack to owner


To Do
=====
- Inconsistency repair


Change Log
==========

- 4/15: project set up
- 4/16: read in user commands
- 4/17: message passing interface working
- 4/20: operations working
- 4/20: owner contacting replicas to do operation and ACK transmitting
- 4/41: delays. consistency level enforcement. new pthread to execute each command received in a message. get command working.

