#include <iostream>
#include <queue>
#include <pthread.h>
#include <random>
#include <unistd.h>
#include <string>
#include <fstream>

// Queues used in hospital modelling.
std::queue<int> receptionQueue;
std::queue<int> dentistQueue;
std::queue<int> surgeonQueue;
std::queue<int> therapistQueue;

// Global variables (initialized at start) and constants.
const int NUMBER_OF_DOCTORS_ON_DUTY = 2;
int inputNumberOfPatients;
std::string outputFileName;
std::ofstream file;

// Mutexes and rwlocks.
pthread_rwlock_t receptionRWLock;
pthread_rwlock_t dentistRWLock;
pthread_rwlock_t surgeonRWLock;
pthread_rwlock_t therapistRWLock;
pthread_mutex_t coutMutex;
pthread_mutex_t fileMutex;
pthread_mutex_t numberOfPatientsMutex;

// Threads.
pthread_t* patientThreads;
pthread_t dutyDoctorsThreads[2];
pthread_t dentistThread = pthread_t();
pthread_t surgeonThread = pthread_t();
pthread_t therapistThread = pthread_t();


int getRandomIntInRange(int lowerBound, int upperBound) { // Random generator in range [lowerBound, upperBound].
    std::random_device randomSeed;
    std::mt19937 intGenerator(randomSeed());
    std::uniform_int_distribution<> dis(lowerBound, upperBound);

    return dis(intGenerator);
}

void joinThreads() { // Joins all threads.
    for (int i = 0; i < inputNumberOfPatients; ++i) {
        pthread_join(patientThreads[i], nullptr);
    }
    pthread_join(dutyDoctorsThreads[0], nullptr);
    pthread_join(dutyDoctorsThreads[1], nullptr);
    pthread_join(dentistThread, nullptr);
    pthread_join(surgeonThread, nullptr);
    pthread_join(therapistThread, nullptr);

}

void cycleMainThread() {
    while (true) {
        pthread_mutex_lock(&numberOfPatientsMutex);
        if (inputNumberOfPatients == 0) { // Checks if hospital is empty.
            pthread_mutex_unlock(&numberOfPatientsMutex);
            joinThreads(); // Joins all threads.
            file.close(); // Closes file.
            break;
        }
        pthread_mutex_unlock(&numberOfPatientsMutex);
        // Each 4 seconds.
        sleep(4);
    }
}

void seeByTherapist() { // Therapist's visit. It takes from 2 to 4 seconds.
    pthread_rwlock_rdlock(&therapistRWLock);

    if (!therapistQueue.empty()) {
        // Seeing the first patient.
        int currentPatientID = therapistQueue.front();
        therapistQueue.pop();

        // Output.
        pthread_mutex_lock(&coutMutex);
        pthread_mutex_lock(&fileMutex);
        std::cout << "Therapist see patient with ID [" << currentPatientID << "]\n";
        file << "Therapist see patient with ID [" << currentPatientID << "]\n";
        pthread_mutex_unlock(&fileMutex);
        pthread_mutex_unlock(&coutMutex);
    }

    pthread_rwlock_unlock(&therapistRWLock);

    // Visit takes from 2 to 4 seconds.
    sleep(getRandomIntInRange(2, 4));
}

void seeBySurgeon() { // Surgeon's visit. It takes from 3 to 5 seconds.
    pthread_rwlock_rdlock(&surgeonRWLock);

    if (!surgeonQueue.empty()) {
        // Seeing the first patient.
        int currentPatientID = surgeonQueue.front();
        surgeonQueue.pop();

        // Output.
        pthread_mutex_lock(&coutMutex);
        pthread_mutex_lock(&fileMutex);
        std::cout << "Surgeon see patient with ID [" << currentPatientID << "]\n";
        file << "Surgeon see patient with ID [" << currentPatientID << "]\n";
        pthread_mutex_unlock(&fileMutex);
        pthread_mutex_unlock(&coutMutex);
    }

    pthread_rwlock_unlock(&surgeonRWLock);

    // Visit takes from 3 to 5 seconds.
    sleep(getRandomIntInRange(3, 5));
}

void seeByDentist() { // Dentist's visit. It takes from 1 to 2 seconds.
    pthread_rwlock_rdlock(&dentistRWLock);

    if (!dentistQueue.empty()) {
        // Seeing the first patient.
        int currentPatientID = dentistQueue.front();
        dentistQueue.pop();

        // Output.
        pthread_mutex_lock(&coutMutex);
        pthread_mutex_lock(&fileMutex);
        std::cout << "Dentist see patient with ID [" << currentPatientID << "]\n";
        file << "Dentist see patient with ID [" << currentPatientID << "]\n";
        pthread_mutex_unlock(&fileMutex);
        pthread_mutex_unlock(&coutMutex);
    }

    pthread_rwlock_unlock(&dentistRWLock);

    // Visit takes from 1 to 2 seconds.
    sleep(getRandomIntInRange(1, 2));
}

/// Doctors thread function. Checks if there are any patients in hospital. Distributes them to doctors according to their diagnosis.
/// \param param Patient diagnosis.
/// \return Nothing.
void *seePatient(void* param) {
    int doctorTypeID = *(int*)param;

    while (true) {
        // Checks if there are patients in hospital.
        pthread_mutex_lock(&numberOfPatientsMutex);
        if (inputNumberOfPatients == 0) { // Usage of patients counter.
            pthread_mutex_unlock(&numberOfPatientsMutex);
            break;
        }
        pthread_mutex_unlock(&numberOfPatientsMutex);

        // Distributes patients.
        switch (doctorTypeID) {
            case 1:
                seeByDentist();
                break;
            case 2:
                seeBySurgeon();
                break;
            case 3:
                seeByTherapist();
                break;
            default:
                std::cout << "Impossible...\n";
                break;
        }
    }
    return nullptr;
}

void makeDiagnosis(int patient) { // Sends patient to one of other doctors queue.
    // Random diagnosis (hope that it doesn't work that way in common hospitals).
    int diagnosis = getRandomIntInRange(1, 3);
    switch (diagnosis) {
        case 1: // Send to dentist.
            pthread_rwlock_wrlock(&dentistRWLock);
            dentistQueue.emplace(patient);
            pthread_rwlock_unlock(&dentistRWLock);
            break;
        case 2: // Send to surgeon.
            pthread_rwlock_wrlock(&surgeonRWLock);
            surgeonQueue.emplace(patient);
            pthread_rwlock_unlock(&surgeonRWLock);
            break;
        case 3: // Send to therapist.
            pthread_rwlock_wrlock(&therapistRWLock);
            therapistQueue.emplace(patient);
            pthread_rwlock_unlock(&therapistRWLock);
            break;
        default: // Impossible branch.
            std::cout << "Impossible...\n";
    }
}

/// Doctors on duty thread function. Sees patients from reception queue and sending them to one of other doctors.
/// Visit takes from 1 to 5 seconds.
/// \param param DoctorsID.
/// \return Nothing.
void *seePatientOnReceptionQueue(void* param) {
    int doctorID = *(int*)param;

    while (true) {
        pthread_rwlock_rdlock(&receptionRWLock);

        if (!receptionQueue.empty()) {
            // See the first patient in queue.
            int currentPatientID = receptionQueue.front();
            receptionQueue.pop();

            // Output.
            pthread_mutex_lock(&coutMutex);
            pthread_mutex_lock(&fileMutex);
            std::cout << "Doctor on duty with ID [" << doctorID << "] see patient with ID [" << currentPatientID << "]\n";
            file << "Doctor on duty with ID [" << doctorID << "] see patient with ID [" << currentPatientID << "]\n";
            pthread_mutex_unlock(&coutMutex);
            pthread_mutex_unlock(&fileMutex);

            // Sends patient to one of other doctors queue.
            makeDiagnosis(currentPatientID);
        }

        pthread_rwlock_unlock(&receptionRWLock);

        // Finishes thread when reception is empty.
        if (receptionQueue.empty()) {
            break;
        }

        // Doctors on duty visit takes from 1 to 5 seconds.
        sleep(getRandomIntInRange(1, 5));
    }
    return nullptr;
}

void fillHospitalWithDoctors() {
    // Initialization.
    int doctors[NUMBER_OF_DOCTORS_ON_DUTY];
    int dentistNumber[1] = {1};
    int surgeonNumber[1] = {2};
    int threapistNumber[1] = {3};

    for (int i = 0; i < NUMBER_OF_DOCTORS_ON_DUTY; ++i) { // Duty doctors threads.
        doctors[i] = i + 1;
        pthread_create(&(dutyDoctorsThreads[i]), nullptr, seePatientOnReceptionQueue, (void*)(doctors + i));
    }

    // Other doctors threads.
    pthread_create(&dentistThread, nullptr, seePatient, (void*)(dentistNumber));
    pthread_create(&surgeonThread, nullptr, seePatient, (void*)(surgeonNumber));
    pthread_create(&therapistThread, nullptr, seePatient, (void*)(threapistNumber));
}

bool isInQueue(int patient, std::queue<int> queue) { // Checks if the patient is in queue.
    while (!queue.empty()) {
        if (patient == queue.front()) {
            return true;
        }
        queue.pop();
    }
    return false;
}

/// Patients thread function. Prints information about patient for every 4 seconds
/// (at least if he is not already at home).
/// \param param PatientID.
/// \return Nothing.
void *getPatientStatus(void* param) {
    int patientID = *(int*)param;

    while (true) {
        // Locks all queues to find patient.
        pthread_rwlock_wrlock(&receptionRWLock);
        pthread_rwlock_wrlock(&dentistRWLock);
        pthread_rwlock_wrlock(&surgeonRWLock);
        pthread_rwlock_wrlock(&therapistRWLock);

        bool isPatientAtHome = false;

        // Locks output mutexes.
        pthread_mutex_lock(&coutMutex);
        pthread_mutex_lock(&fileMutex);
        if (isInQueue(patientID, receptionQueue)) {
            std::cout << "Patient with ID [" << patientID << "] waits in reception queue\n";
            file << "Patient with ID [" << patientID << "] waits in reception queue\n";
        } else if (isInQueue(patientID, dentistQueue)) {
            std::cout << "Patient with ID [" << patientID << "] waits in dentist queue\n";
            file << "Patient with ID [" << patientID << "] waits in dentist queue\n";
        } else if (isInQueue(patientID, surgeonQueue)) {
            std::cout << "Patient with ID [" << patientID << "] waits in surgeon queue\n";
            file << "Patient with ID [" << patientID << "] waits in surgeon queue\n";
        } else if (isInQueue(patientID, therapistQueue)) {
            std::cout << "Patient with ID [" << patientID << "] waits in therapist queue\n";
            file << "Patient with ID [" << patientID << "] waits in therapist queue\n";
        } else {
            std::cout << "Patient with ID [" << patientID << "] is already at home\n";
            file << "Patient with ID [" << patientID << "] is already at home\n";
            isPatientAtHome = true;
        }
        // Unlocks output mutexes.
        pthread_mutex_unlock(&coutMutex);
        pthread_mutex_unlock(&fileMutex);

        // Unlocks queues.
        pthread_rwlock_unlock(&receptionRWLock);
        pthread_rwlock_unlock(&dentistRWLock);
        pthread_mutex_unlock(&surgeonRWLock);
        pthread_mutex_unlock(&therapistRWLock);

        // Finishes thread.
        if (isPatientAtHome) {
            pthread_mutex_lock(&numberOfPatientsMutex);
            --inputNumberOfPatients;
            pthread_mutex_unlock(&numberOfPatientsMutex);
            break;
        }

        // Each 4 seconds.
        sleep(4);
    }
    return nullptr;
}

void fillReceptionWithPatients() {
    patientThreads = new pthread_t[inputNumberOfPatients]; // Threads.
    int* patients = new int[inputNumberOfPatients]; // IDs.

    for (int i = 0; i < inputNumberOfPatients; ++i) {
        receptionQueue.emplace(i + 1); // Filling the queue.
        patients[i] = i + 1;
        pthread_create(&patientThreads[i], nullptr, getPatientStatus, (void*)(patients + i));
    }
}

void initializeMutexesAndRWLocks() { // Initialize all mutexes and rwlocks.
    pthread_rwlock_init(&receptionRWLock, nullptr);
    pthread_rwlock_init(&dentistRWLock, nullptr);
    pthread_rwlock_init(&surgeonRWLock, nullptr);
    pthread_rwlock_init(&therapistRWLock, nullptr);
    pthread_mutex_init(&coutMutex, nullptr);
    pthread_mutex_init(&fileMutex, nullptr);
    pthread_mutex_init(&numberOfPatientsMutex, nullptr);
}

void getInputDataFromCommandLine(char* argv[]) {
    inputNumberOfPatients = std::stoi(argv[1], nullptr, 10);
    outputFileName = argv[2];
}

void getInputDataFromFile(char* argv[]) {
    std::string inputFile = argv[2];
    std::ifstream input;
    input.open(inputFile);
    input >> inputNumberOfPatients >> outputFileName;
}

bool isFileInput(char* argv[]) {
    return *argv[1] == *"-i";
}

void inputFromCommandLine(char* argv[]) {
    if (isFileInput(argv)) { // checks if marked as "-i".
        getInputDataFromFile(argv);
    } else {
        getInputDataFromCommandLine(argv);
    }
}

bool isCommandCorrect(int argc) {
    return argc == 3;
}

int main(int argc, char* argv[]) {
    if (!isCommandCorrect(argc)) { // Checks command length.
        std::cout << "Incorrect input format";
        return 0;
    }
    inputFromCommandLine(argv); // Initializes inputNumberOfPatients and outputFileName.
    initializeMutexesAndRWLocks(); // Mutex and rwlocks Initialization.
    file.open(outputFileName); // Open file for output.

    fillReceptionWithPatients(); // Fill reception queue and create patients threads.
    fillHospitalWithDoctors(); // Creates doctors threads.
    cycleMainThread(); // Cycles main thread.
}
