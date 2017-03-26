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

(define map
    (lambda (fn list)
      (if (nullp list)
          nil
          (cons (fn (car list))
                (map fn (cdr list))))))

(map fact (quote (4 5 6)))

(define member
    (lambda (elt list)
      (if (nullp list)
          nil
          (or (eql elt (car list))
              (member elt (cdr list))))))

(member 5 (quote (4 5 6)))
