/*

Names: Steven Jiao, Tyler Mah
CCID: 1442672, 1429548
CMPUT 274, Fall 2019
Date: 11/02/2019
Acknowledgements: Fast GCD Function from in class and upper_sqrt from
"Primality Testing" morning problem. 
Title: Assignment 2 Part 2: Encrypted Communication

*/

#include <Arduino.h>

// Set client states
enum clientState {
    Start, WaitingForAck, cDataExchange
};
// Set server states
enum serverState {
    Listen, WaitingForKey, WaitForAck, WaitingForKey2, WaitForAck2, DataExchange
};


void setup() {
    // begin both ports and pin13, and pin1
    init();
    Serial.begin(9600);
    Serial3.begin(9600);
    pinMode(13, INPUT);

}

// computes the value (a * b) mod m of 2 uint32 types, 
uint32_t mulmod(uint32_t a, uint32_t b, uint32_t m) {
  uint32_t ans = 0;
  // base case of (2^0 * b) mod m
  uint32_t multi = b % m;

  // loop through a bitwise to add (a(i) * b) mod m
  while (a > 0) {
    // if the lsb of a is 1, add the (2^i * b) mod m
    if ((a & 1) == 1) {
      ans = (ans + multi) % m;
    }
    // keep multiplying by 2 for 2^(i+1) mod m
    multi = (2 * multi) % m;
    // divide a by 2
    a >>= 1; 
  }
  return ans;
}

// computes the value x^pow mod m ("x to the power of pow" mod m)
uint32_t powmod(uint32_t x, uint32_t pow, uint32_t m) {
  uint32_t ans = 1;
  uint32_t pow_x = x;

  while (pow > 0) {
    if ((pow & 1) == 1) {
      ans = mulmod(ans,pow_x,m);
    }

    pow_x = mulmod(pow_x,pow_x,m);

    pow >>= 1; // divides by 2
  }

  return ans;
}

/** Writes an uint32_t to Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
void uint32_to_serial3(uint32_t num) {
    Serial3.write((char) (num >> 0));
    Serial3.write((char) (num >> 8));
    Serial3.write((char) (num >> 16));
    Serial3.write((char) (num >> 24));
}

/** Reads an uint32_t from Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
uint32_t uint32_from_serial3() {
    uint32_t num = 0;
    num = num | ((uint32_t) Serial3.read()) << 0;
    num = num | ((uint32_t) Serial3.read()) << 8;
    num = num | ((uint32_t) Serial3.read()) << 16;
    num = num | ((uint32_t) Serial3.read()) << 24;
    return num;
}

/** server is the function that implements the communication encryption
* and is used by both client and server to encode, and decode messages.
* server takes in the parameters of the privatekey, mod, and the encoding 
* key and modulus. 
*/
void runChat(uint32_t myPrivateKey, uint32_t myMod, uint32_t outPublicKey, uint32_t outMod) {
    // main loop for the encrypted communication
    while (true) {
        // check if theres a char/byte to read from serial
        if (Serial.available() > 0) {
            // get the char
            uint32_t byte_read = Serial.read();
            
            // write to serial
            Serial.write(byte_read);

            // if /r is pressed, follow with a /n
            if (byte_read == 13) {
                // write a newline to serial
                Serial.write(10);
                // encode the /r to serial3
                uint32_t enc_byte = powmod(byte_read, outPublicKey, outMod);
                uint32_to_serial3(enc_byte);

                // encode and send the /n to serial3 as well
                uint32_t new_line = powmod(10, outPublicKey, outMod);
                uint32_to_serial3(new_line);
            } else {
                // encode only whats sent
                uint32_t enc_byte = powmod(byte_read, outPublicKey, outMod);
                uint32_to_serial3(enc_byte);
            }

        // check if theres a uint32 to decode from serial3
        } else if (Serial3.available() >= 4) {
            // if there is, read and decode
            uint32_t byte_read = uint32_from_serial3();
            uint32_t dec_byte = powmod(byte_read, myPrivateKey, myMod);
            Serial.write(dec_byte);    
        }   
    }
}


/* --------------------- IMPLEMENTATION OF PART 2 -------------------------- */


/** From the morning problem "Primality Testing": 
* Given a positive integer n <= 4,000,000 this returns the smallest
* integer d such that d*d > n
*/
unsigned int upper_sqrt(unsigned int n) {
    unsigned int d = sqrt((double) n);

    // should iterate at most once or twice
    // the second condition is to protect against overflow
    // if n is very close to the maximum 32-bit integer
    while (d*d <= n && d <= (1<<16)) {
        ++d;
    }

    return d;
}


/** From in class, the fast-GCD method by elucian
* Finds the greated common divisor between intput integers a and b
*/
uint32_t gcd(uint32_t a, uint32_t b) {
  while (b > 0) {
    a %= b;

    // now swap them
    uint32_t tmp = a;
    a = b;
    b = tmp;
  }
  return a;
}

/** This function is based off my function from "Primality Testing":
* isPrime checks if a number n is a prime or not. Returns true if it is, and 
false otherwise.
*/
bool isPrime(uint32_t n) {
    // If n is less than or equal 1, it is not a prime.
    if (n <= 1) {
        return false;
    // else, loop up to sqrt(n) to see if n is prime.
    } else {
        int d = upper_sqrt(n);
        for (int i = 2; i < d; i++) {
            if ((n % i) == 0) {
                return false;
            }
        }
        return true;
    }
}

/** primeRand(n) will return an n-bit randomly generated number via 
* analogRead() of pin A1 that is also a prime.
*/
uint32_t primeRand(uint32_t n) {
    // initialize return variable
    uint32_t randbytes = 0;
    // loop through as many bits as specified
    for (uint32_t i = 0; i < n; i++) {
        // get a reading of pin1
        uint32_t reading = analogRead(A1);
       
        // check if its 0 or 1 and append to randbyte
        if ((reading & 1) == 1) {
            randbytes <<= 1;
            randbytes++;
        } else {
            randbytes <<= 1;
        }
        // delay 5ms
        delay(5);
    }
    randbytes = randbytes | (1L<<n);

    if (isPrime(randbytes) == true) {
        return randbytes;
    } else {
        primeRand(n);
    }
}

// Given an integer x, possibly negative, return an integer 
// in the range 0..m-1 that is congruent to x (mod m) 
int32_t reduce_mod(int32_t x, uint32_t m) {
    int32_t mod = m;
    // if x is negative, mod it to get the smallest negative 
    if (x < 0) {
        x %= mod;
        // if x is still negative, add the mod to x to make it positive
        if (x < 0) {
            x += mod;
        }
        return x;
    } else {
        return (x % mod);
    }
}

/** get_d takes e and phi(n) (or a, b) and calculates a d such that 
* e * d % phi(n) = 1
*/
uint32_t get_d(uint32_t a,uint32_t b) {
    // initialize matricies.
    int32_t r[40] = {(int32_t) a, (int32_t) b};
    int32_t t[40] = {0, 1};
    int32_t s[40] = {1, 0};
    int32_t q;

    // initialize index
    uint32_t i = 1;

    // perform algorithm as specified on eclass to obtain d.
    while (r[i] > 0) {
        q = r[i-1] / r[i];
        r[i+1] = r[i-1] - q*r[i];
        s[i+1] = s[i-1] - q*s[i];
        t[i+1] = t[i-1] - q*t[i];
        i++;
    }
    // store d 
    int32_t x = s[i-1];

    // return it this way to ensure a positive d is obtained
    return (uint32_t) reduce_mod(x,b);
}

 
/** generateKeys will take pointer inputs of n, e and d and change them to 
* have primes allocated to these variables.
*/ 
void generateKeys(uint32_t& n, uint32_t& e, uint32_t& d) {
    // Hopefully this is a helpful indicator of where the program is at
    Serial.println("Generating Keys...");

    // generate random prime of 2^14 for p
    uint32_t p = primeRand(14);

    // generate random prime of 2^15 for p
    uint32_t q = primeRand(15);

    // get n
    n = p*q;

    // get phi(n)
    uint32_t phi_n = (p-1)*(q-1);

    // get e
    e = primeRand(15);

    while (gcd(e,phi_n) != 1) {
        e = primeRand(15);
    }

    d = get_d(e, phi_n);

    // TESTER PRINTOUTS: print the values obtained 
    Serial.print("p: "); 
    Serial.println(p); 
    Serial.print("q: ");
    Serial.println(q);
    Serial.print("n: ");
    Serial.println(n);
    Serial.print("Phi(n): ");
    Serial.println(phi_n); 
    Serial.print("e: "); 
    Serial.println(e); 
    Serial.print("d: "); 
    Serial.println(d); 



/** Waits  for a certain  number  of  bytes on  Serial3  or  timeout
* @param  nbytes: the  number  of  bytes we want
* @param  timeout: timeout  period (ms); specifying a negative  number
*                   turns  off  timeouts (the  function  waits  indefinitely
*                   if  timeouts  are  turned  off).
* @return  True if the  required  number  of bytes  have  arrived.
*/
bool wait_on_serial3( uint8_t  nbytes , long  timeout ) {
    unsigned  long  deadline = millis () + timeout; // wraparound  not a problem
    while (Serial3.available()<nbytes  && (timeout <0 ||  millis ()<deadline)) 
    {
    delay (1);
    }
    return  Serial3.available () >=nbytes;
}


/** client() is used to run the client side of communcation. 
* requires the client key and mod, and its private key.
*/
void client(uint32_t cKey, uint32_t cMod, uint32_t cPrivateKey) {
    // Visual indicator of which computer is what
    Serial.println();
    Serial.println("Client-Side Communication");
    Serial.println();

    // initial client state
    clientState state = Start;
    // initialize server key and server mod to store 
    uint32_t sKey;
    uint32_t sMod;
    // loop initiate handshake until client is ready
    while(state != cDataExchange) {
        // send C and keys and mod if in start
        if (state == Start) {
            Serial.println("Client in Start/Ack Phase"); // test
            Serial3.write('C');
            uint32_to_serial3(cKey);
            uint32_to_serial3(cMod);
            state = WaitingForAck;  
        // Wait for acknowledgement by getting server keys and mod
        } else if (state == WaitingForAck) {
            if (wait_on_serial3(9,1000) && Serial3.read() == 'A') {
                sKey = uint32_from_serial3();
                sMod = uint32_from_serial3();
                Serial3.write('A');
                state = cDataExchange;
            // if this doesn't happen in 1s, start over
            } else {
                state = Start;
            }
        }
    }
    // data exchange
    runChat(cPrivateKey, cMod, sKey, sMod);
}

/** server() is used to run the server side of communcation. 
* requires the server key and mod, and its private key.
*/
void server(uint32_t sKey, uint32_t sMod, uint32_t sPrivateKey) {
    // Server's visual indicator
    Serial.println();
    Serial.println("Server-Side Communication");
    Serial.println();

    serverState state = Listen;
    // initialize client key store
    uint32_t cKey;
    uint32_t cMod;
    // loop until data exchange
    while (state != DataExchange) {
        // if in listen state, wait for client keys
        if (state == Listen) {
            Serial.println("Server in Listen Phase");
            if (wait_on_serial3(1, 1000) && Serial3.read() == 'C') {
                state = WaitingForKey;
            } 
        // after getting keys, exchange server keys and store client keys
        } else if (state == WaitingForKey) {
            Serial.println("Server in Waiting for Key Phase"); // test
            if (wait_on_serial3(8,1000)) {
                cKey = uint32_from_serial3();
                cMod = uint32_from_serial3();
                Serial3.write('A');
                uint32_to_serial3(sKey);
                uint32_to_serial3(sMod);
                state = WaitForAck;
            } else { 
                state = Listen;
            }
        // wait for acknowledgement 
        } else if (state == WaitForAck) {
            Serial.println("Server in Waiting for Ack Phase"); // test
            if (wait_on_serial3(1,1000)) {
                uint32_t msg = Serial3.read();
                if (msg == 'A') {
                    state = DataExchange;
                } else if (msg == 'C') {
                    state = WaitingForKey2;
                } else {
                    state = Listen;
                }
            } else {
                state = Listen;
            }
        // backup wait for key phase just in case
        } else if (state == WaitingForKey2) {
            Serial.println("Server in Waiting for Key2 Phase"); // test
            if (wait_on_serial3(8, 1000)) {
                cKey = uint32_from_serial3();
                cMod = uint32_from_serial3();
                state = WaitForAck2;
            } else {
                state = Listen;
            }
        // backup wait for key phase goes to wait for acknowledge 
        } else if (state == WaitForAck2) {
            Serial.println("Server in Waiting for Ack2 Phase"); // test
            if (wait_on_serial3(1, 1000)) {
                uint32_t msg = Serial3.read();
                if (msg == 'A') {
                    state = DataExchange;
                } else if (msg == 'C') {
                    state = WaitingForKey2;
                } else {
                    state = Listen;
                }
            } else {
                state = Listen;
            }
        }   
    }
    // data exchange
    runChat(sPrivateKey, sMod, cKey, cMod);
}

int main() {
    setup();

    // initialize n, e, and d for this computer 
    uint32_t n;
    uint32_t e;
    uint32_t d;

    // pass n, e and d as pointers through generateKeys 
    generateKeys(n,e,d);

    // if pin13 shows HIGH; will be server. Else, be client
    if (digitalRead(13) == HIGH) {
        server(e, n, d);
    } else {
        client(e, n, d);
    }



    // flush serial and return 0
    Serial.flush();
    Serial3.flush();
    return 0;
}

