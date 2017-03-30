(define seq
  (lambda (start end)
    (if (> start end)
        nil
      (cons start (seq (+ start 1) end)))))

(seq 1 2)
(seq 1 2)
(seq 1 2)
(seq 1 2)
(seq 1 2)
