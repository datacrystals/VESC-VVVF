
;; Compiled C Binary Code
%COMPILED_C_BINARY
;; Load the compiled C code
(load-native-lib vvvf)

;; define motor current and rpm globally so it shows up in the vesc debugger with the values when polling
(def motor-current 0)
(def motor-rpm 0)


;; Function to update amplitude and command frequency
(defun update-audio-parameters ()
  (progn
    (def motor-current (abs (get-current)))
    (def motor-rpm (get-rpm))

    
    ; (ext-set-amplitude amplitude)
    (ext-set-motor-current (+ 0 motor-current))
    (ext-set-motor-hz (+ 0 motor-rpm))
    (ext-set-motor-poles 14) ; I think i'ts 14 for a qs205 but idk for sure...
  )

)

;; Start the audio loop
(ext-start-audio-loop)

;; Debug: Print a message before entering the main loop
(print "Entering main loop...")

;; Main loop to update parameters every 20ms
(loopwhile t {
  (update-audio-parameters)

  ;; OPTIONAL FOR DEBUGGING
  (ext-get-stats)

  (sleep 0.02) ; Sleep for 20ms
})