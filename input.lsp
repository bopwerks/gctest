(define make-withdraw
  (lambda (balance)
    (lambda (n) (set! balance (- balance n)))))
(define w (make-withdraw 400))
(w 5)
(w 5)
