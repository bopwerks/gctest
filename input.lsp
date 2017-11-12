(define (make-person name)
  (define (get-name)
    name)
  (define (dispatch msg)
    (if (eql msg (quote name))
        get-name))
  dispatch)

(define p (make-person (quote jesse)))
(print (quote (after defining person)))
p
(define (name person)
  ((person (quote name))))
(name p)
