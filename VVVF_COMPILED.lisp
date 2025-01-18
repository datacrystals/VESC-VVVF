(def chunk-size 200) ; Define the chunk size
(def pi 3.1415926535) ; Basic pi definition

;; Create buffers
(def buf1 (bufcreate chunk-size))
(def buf2 (bufcreate chunk-size))
(def buf3 (bufcreate chunk-size))

;; Global phase angles
(def command-phase 0.0) ; Phase for command signal
(def carrier-phase 0.0) ; Phase for carrier signal

;; Sample rate (Hz)
(def sample-rate 11025.0) ; THIS MUST HAVE THE DECIMAL IN IT SO IT'S A FLOAT!!

;; Motor current range (in amps)
(def min-current -10.0) ; Minimum motor current for mapping
(def max-current 100.0) ; Maximum motor current for mapping

;; Output voltage range (capped at 0.3)
(def min-voltage 0.01) ; Minimum output voltage for mapping
(def max-voltage 0.1) ; Maximum output voltage for mapping (capped at 0.3)

;; Processing Variables
(def motor_current 0.0) ; Current amperes of the motor
(def motor_rpm 0.0) ; Current rpm of motor
(def amplitude 0.0) ; Initialize amplitude to 0.0

;; PWM settings (as globals)
(def spwm-type 'fixed) ; Can be 'fixed, 'ramp, 'sync, or 'rspwm
(def spwm-carrier-frequency 1000) ; For 'fixed mode
(def spwm-min-carrier-frequency 500) ; For 'ramp and 'rspwm modes
(def spwm-max-carrier-frequency 2000) ; For 'ramp and 'rspwm modes
(def spwm-pulse-mode 10) ; For 'sync mode (number of pulses per command cycle)
(def spwm-wide-pulse nil) ; Enable wide pulse mode
(def spwm-modulation-index 1.5) ; Modulation index for wide pulse mode


;; Compiled C Binary Code
(def vvvf [
0x00 0x00 0x00 0x00 0x08 0xb5 0x07 0x4b 0x07 0x49 0x08 0x48 0x7b 0x44 0x79 0x44 0x1b 0x68 0x03 0x4b
0x78 0x44 0x1b 0x68 0x98 0x47 0x01 0x20 0x08 0xbd 0x00 0xbf 0x00 0xf8 0x00 0x10 0xf0 0xff 0xff 0xff
0x2b 0x00 0x00 0x00 0x18 0x00 0x00 0x00 0x65 0x78 0x74 0x2d 0x74 0x65 0x73 0x74 0x00 0x00 0x00 0x00
0x01 0x29 0x70 0xb5 0x09 0x4d 0x04 0x46 0x0d 0xd1 0xeb 0x6f 0x00 0x68 0x98 0x47 0x48 0xb1 0x6b 0x6e
0x20 0x68 0x2e 0x6c 0x98 0x47 0x33 0x46 0x00 0xeb 0x40 0x00 0xbd 0xe8 0x70 0x40 0x18 0x47 0xd5 0xf8
0x94 0x00 0x70 0xbd 0x00 0xf8 0x00 0x10 
])



;; Load the compiled c code
(load-native-lib vvvf)

(print (ext-test 1))








(defun map-value (value in-min in-max out-min out-max)
  ; "Maps a value from one range to another."
  (+ out-min (* (/ (- value in-min) (- in-max in-min)) (- out-max out-min))))

(defun generate-command (frequency amplitude phase)
  ; "Generates a command signal (sine wave)."
  (def angular-frequency (* 2 pi (/ frequency sample-rate))) ; Replaced let with def
  (* amplitude (sin (+ phase angular-frequency))))

(defun generate-carrier (frequency mode phase)
  ; "Generates a carrier signal (sawtooth or triangle wave)."
  (def angular-frequency (* 2 pi (/ frequency sample-rate))) ; Replaced let with def
  (case mode
    ('sawtooth (mod (+ phase angular-frequency) pi)) ; Sawtooth wave
    ('triangle (abs (- (* 2 (/ (mod (+ phase angular-frequency) pi) pi)) 1))) ; Triangle wave
    (t 0))) ; Default to 0

(defun generate-output (command carrier power-rail)
  ; "Generates the output signal based on the command and carrier signals."
  (cond
    ((and (> command 0) (> command carrier)) power-rail)
    ((and (< command 0) (< command (- carrier))) (- power-rail))
    (t 0)))

(defun get-carrier-frequency (erpm)
  ; "Calculates the carrier frequency based on the selected PWM mode."
  (case spwm-type
    ('fixed spwm-carrier-frequency) ; Fixed frequency
    ('ramp (+ spwm-min-carrier-frequency
               (* (/ (- erpm min-speed) (- max-speed min-speed))
                  (- spwm-max-carrier-frequency spwm-min-carrier-frequency)))) ; Ramp between min and max frequencies
    ('sync (* spwm-pulse-mode (/ (abs erpm) 60))) ; Sync with command frequency
    ('rspwm (+ spwm-min-carrier-frequency
                (* (random 1.0)
                   (- spwm-max-carrier-frequency spwm-min-carrier-frequency)))) ; Random switching
    (t spwm-carrier-frequency))) ; Default to fixed frequency




;(defun get-samples (buf)
  ;(progn
    ;(def frequency 500) ; 1 kHz sine wave
    ;(def angular-frequency (* 2 pi (/ frequency sample-rate))) ; Angular frequency
    (var phase carrier-phase)
    ;(def i 0) ; Initialize loop counter
    ;(loopwhile (< i chunk-size) ; Loop condition
      ;(progn
        ;(var sample (round (* 127 (sin carrier-phase)))) ; Calculate sine wave sample using current phase
        ;(bufset-i8 buf1 i sample) ; Write the sample to the buffer
        ;(def carrier-phase (+ carrier-phase angular-frequency)) ; Update phase angle for the next sample
        ;(def i (+ i 1)) ; Increment loop counter
      ;)
    ;)
    (def carrier-phase phase)
  ;)
  ;buf
;)



(defun play-samples (buf) {
        (get-samples buf)

        (foc-play-samples buf sample-rate 0.03)
})



;; Main loop
(loopwhile t {
  ;; -- UPDATE RPM AND MOTOR CURRENT --
  (def motor_current (abs (get-current)))
  (def motor_rpm (get-rpm))

  ;; -- NOW, USING ACQUIRED DATA, CALCULATE AMPLITUDE --
  (def amplitude 0.0) ; Default to 0 amplitude unless otherwise specified
  (if (> (abs motor_current) 1)
      ; Map the range of amplitudes from the min current to max current across the min to max voltage
      (def amplitude (map-value (abs motor_current) min-current max-current min-voltage max-voltage)))

  ;; -- PLAY SAMPLES --
  (if (> amplitude -0.1)
      (progn
        (play-samples buf1)
        (play-samples buf2)
        (play-samples buf3)

       )
   )

})