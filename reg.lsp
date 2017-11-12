(+ 2 2)
(define square1 (lambda (x) (* x x)))
(define (square2 n) (* n n))
(square1 5)
(square2 6)
(define make-withdraw1
  (lambda (balance)
    (lambda (n) (set! balance (- balance n)))))

(define w1 (make-withdraw1 400))
(w1 5)
(w1 5)

(define (make-withdraw2 balance)
  (lambda (n) (set! balance (- balance n))))

(define w2 (make-withdraw2 400))
(w2 5)
(w2 5)
