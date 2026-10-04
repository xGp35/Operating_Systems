
while(STATUS == BUSY){
    ; // spin - Wait until device is not busy. This is called polling the device. We repeatedly read the Status Register.
}
Write "data" to DATA Register
Write "command" to COMMAND Register
    // This starts the device and executes the command
while(STATUS == BUSY) {
    ; // spin - Wait until device is done with your request
}