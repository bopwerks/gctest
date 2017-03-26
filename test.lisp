(define square
    (lambda (x) (* x x)))

(square 25)

(define min
    (lambda (a b)
      (if (< a b) a b)))

(min 4 3)

(define fact
    (lambda (n)
      (if (= n 1)
          1
          (* n (fact (- n 1))))))

(fact 6)
