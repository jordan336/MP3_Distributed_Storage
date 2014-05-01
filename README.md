MP3_Distributed_Storage
=======================

ECE 428 - Distributed Systems
Distributed Storage MP

- Mark Kennedy - kenned31
- Jordan Ebel  - ebel1


Compile
=======
Inside /src directory:
-make
-make clean


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
- Owner process with level <= 1   :  receive message -> create pthread -> do command -> contact replicas -> reply to original process -> create pthread and handle replica acks in background
- Replica process with any level  :  receive message -> create pthread -> do command -> send ack to owner


Algorithms and Implementation
=============================

Background:
	Communication is completed using UDP sockets.  Every process has two threads to read commands typed from the user, and to read commands sent from other processes.  Whenever a command is 
typed in or received, a new pthread executes the command.

Command execution steps:
	When a command is typed into a terminal, that terminal parses the input and contacts the owner of the requested key.  The terminal passes the command to the owner and waits for the response.  
The owner receives the message and creates a new pthread to execute the command and contact two replicas.  If the consistency level is ALL, the owner waits for the replicas to respond before sending
the result to the original terminal.  If the consistency level is ONE, the owner immediately sends the result to the original terminal, and creates a new pthread to wait for the replicas to respond.
A replica always receives a message, creates a new pthread to execute the command, then responds to the owner.

Timestamps:
	Each process has a Lamport timestamp, which is used to timestamp every key that process owns.  Every replica stores the key, its value, and the owner's timestamp.  When completing any
command, the owner sends its current Lamport timestamp along with the command to each replica, so the replica can store the Lamport timestamp of the owner along with the key and value.

Consistency Repair:
	A read repair system is used, with the last-writer-win rule.  In the case of a read request from the user, the owner always gets every value from all replicas regardless of the consistency 
level.  The owner checks if the values returned by all replicas match.  If there is a mismatch, the owner selects the most recent write value and updates every replica's copy to reflect the most 
recent value.  When deciding which value is the most recent, the owner will check every timestamp for the greatest number, which corresponds to the most recent write that arrived at the owner.


