; Actual example from man page

              .word $1000
              * = $1000

              ; this is your code at $1000
          part1       rts
              ; this label marks the end of code
          endofpart1

              ; DON'T PUT A NEW .word HERE!
              * = $2000
              .dsb (*-endofpart1), 0
              ; yes, set it again
              * = $2000

              ; this is your code at $2000
          part2       rts

