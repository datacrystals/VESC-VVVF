
;; Compiled C Binary Code
%COMPILED_C_BINARY

;; Load the compiled C code
(load-native-lib vvvf)

;; Define constants
(def chunk-size 400) ; Define the chunk size
(def pi 3.1415926535) ; Basic pi definition

;; Motor current range (in amps)
(def min-current -10.0) ; Minimum motor current for mapping
(def max-current 100.0) ; Maximum motor current for mapping

;; Output voltage range (capped at 0.3)
(def min-voltage 0.01) ; Minimum output voltage for mapping
(def max-voltage 0.1) ; Maximum output voltage for mapping (capped at 0.3)

(def amplitude 0.0)
(def motor-current 0.0)
(def motor-rpm 0.0)

;; Function to map a value from one range to another
(defun map-value (value in-min in-max out-min out-max)
  (+ out-min (* (/ (- value in-min) (- in-max in-min)) (- out-max out-min))))

;; Function to update amplitude and command frequency
(defun update-audio-parameters ()
  (def motor-current (abs (get-current)))
  (def motor-rpm (get-rpm))

  ;; Calculate amplitude based on motor current
  (def amplitude 0.0) ; Default to 0 amplitude unless otherwise specified
  (if (> (abs motor-current) -1)
      (def amplitude (map-value (abs motor-current) min-current max-current min-voltage max-voltage)))
  (def amplitude 0.03)

  ;; Calculate command frequency based on motor RPM
  (def command-frequency (* motor-rpm 0.1)) ; Example scaling factor

  ;; Send updated parameters to the C code
  (ext-set-amplitude amplitude)
  (ext-set-command-frequency command-frequency))

;; Start the audio loop
(ext-start-audio-loop)

;; Main loop to update parameters every 20ms
(loopwhile t {
  (update-audio-parameters)
  (sleep 0.02) ; Sleep for 20ms
})