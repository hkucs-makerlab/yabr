/**
 * consoleCommand - A Wiring/Arduino library to tokenize and parse commands
 * received over a console port.
 * 
 * Copyright (C) 2012 Stefan Rado
 * Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
 *                    http://husks.wordpress.com
 * 
 * Version 20120522
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <SoftwareSerial.h>
#include "SerialCommand.hpp"
template class SerialCommand<HardwareSerial, HardwareSerial>;
template class SerialCommand<HardwareSerial, SoftwareSerial>;
template class SerialCommand<SoftwareSerial, SoftwareSerial>;
template class SerialCommand<SoftwareSerial, HardwareSerial>;
/**
 * Constructor makes sure some things are set.
 */
template<typename T, typename T2>
SerialCommand<T, T2>::SerialCommand(T& bluetooth, T2& console)
  : bluetooth(bluetooth),
    console(console), 
    commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
template<typename T, typename T2>
void SerialCommand<T, T2>::addCommand(const char *command, void (*function)()) {
  #ifdef SERIALCOMMAND_DEBUG
    console.print("Adding command (");
    console.print(commandCount);
    console.print("): ");
    console.println(command);
  #endif

  commandList = (SerialCommandCallback *) realloc(commandList, (commandCount + 1) * sizeof(SerialCommandCallback));
  strncpy(commandList[commandCount].command, command, SERIALCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function = function;
  commandCount++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
template<typename T, typename T2>
void SerialCommand<T, T2>::setDefaultHandler(void (*function)(const char *)) {
  defaultHandler = function;
}


/**
 * This checks the console stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
template<typename T, typename T2>
void SerialCommand<T, T2>::readSerial() {
  while (bluetooth.available() > 0) {
    char inChar = bluetooth.read();   // Read single available character, there may be more waiting
    #ifdef SERIALCOMMAND_DEBUG
      console.print(inChar);   // Echo back to console stream
    #endif

    if (inChar == term) {     // Check for the terminator (default '\r') meaning end of command
      #ifdef SERIALCOMMAND_DEBUG
        console.print("Received: ");
        console.println(buffer);
      #endif

      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
      if (command != NULL) {
        boolean matched = false;
        for (int i = 0; i < commandCount; i++) {
          #ifdef SERIALCOMMAND_DEBUG
            console.print("Comparing [");
            console.print(command);
            console.print("] to [");
            console.print(commandList[i].command);
            console.println("]");
          #endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, SERIALCOMMAND_MAXCOMMANDLENGTH) == 0) {
            #ifdef SERIALCOMMAND_DEBUG
              console.print("Matched Command: ");
              console.println(command);
            #endif

            // Execute the stored handler function for the command
            (*commandList[i].function)();
            matched = true;
            break;
          }
        }
        if (!matched && (defaultHandler != NULL)) {
          (*defaultHandler)(command);
        }
      }
      clearBuffer();
    }
    else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < SERIALCOMMAND_BUFFER) {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      } else {
        #ifdef SERIALCOMMAND_DEBUG
          console.println("Line buffer is full - increase consoleCOMMAND_BUFFER");
        #endif
      }
    }
  }
}

/*
 * Clear the input buffer.
 */
template<typename T, typename T2>
void SerialCommand<T, T2>::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
template<typename T, typename T2>
char *SerialCommand<T, T2>::next() {
  return strtok_r(NULL, delim, &last);
}
