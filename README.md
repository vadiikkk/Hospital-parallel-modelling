## Problem: The task of the hospital
In the hospital, two doctors on duty receive patients, listen to their complaints and send them either to the dentist, or to the surgeon, or to the therapist. Dentist, surgeon and therapist treat patients. Each doctor can only see one patient at a time. Patients stand in line to see doctors and never leave them. Create a multithreaded application that simulates the clinic's working day. Each of the doctors and the patient should be modeled in a separate flow.

## Model
Patients are in a queue at the reception, from where they are called in turn to the two doctors on duty. Each of them randomly diagnoses and sends the patient to one of the three specialist doctors in the queue. Specialists, in turn, treat patients, and they are sent home. The work of all five doctors takes place in parallel – each checks his turn for the presence of patients in it and, if available, serves them. Patients should also be implemented as streams, because in my model, patients will track their status: at what stage they are.
<div align=center>
  <img src = https://github.com/vadiikkk/Hospital-parallel-modelling/assets/132217692/2bb9cbb4-02a7-470e-8220-34fdb4fe7d8b />
</div>

## Test and input data
The solution contains 5 test configuration files each (input1-5.txt ) with all kinds of test cases. The results of the runs are saved accordingly in output1-5.txt Testing can also be done independently: my program supports testing with input data from the command line and from the configuration file.


**Important:** the program implements a minimum check for correct input data (number of arguments on the command line == 3). The input parameters are: the number of patients (range is a non-negative integer) and the name of the output file (format name.txt ).
Examples of acceptable testing by the program:

![image](https://github.com/vadiikkk/Hospital-parallel-modelling/assets/132217692/63425479-e015-425e-9dda-b2cab04d605e)
![image](https://github.com/vadiikkk/Hospital-parallel-modelling/assets/132217692/92e3549d-68c3-45d1-932b-844e99983fc1)
Where input1.txt is:

![image](https://github.com/vadiikkk/Hospital-parallel-modelling/assets/132217692/fb704234-3b36-4d12-a6e9-f879d2b1e881)
