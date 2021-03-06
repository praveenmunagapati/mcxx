! <testinfo>
! test_generator="config/mercurium-fortran run"
! </testinfo>
PROGRAM P
    INTEGER, PARAMETER :: A1(5, 4) = RESHAPE( (/ (I, I=1,10) /), (/ 5, 4 /), PAD = (/ 99, 44 /) )
    INTEGER, PARAMETER :: A2(5, 4) = RESHAPE( (/ (I, I=1,9) /), (/ 5, 4 /), PAD = (/ 99, 44 /) )
    INTEGER, PARAMETER :: A3(5, 4) = RESHAPE( (/ (I, I=1,10) /), (/ 5, 4 /), PAD = (/ 99, 44 /), ORDER = (/ 1, 2 /))
    INTEGER, PARAMETER :: A4(5, 4) = RESHAPE( (/ (I, I=1,9) /), (/ 5, 4 /), PAD = (/ 99, 44 /), ORDER = (/ 2, 1 /))

    CHARACTER(LEN=62) :: TEST_STR_1
    CHARACTER(LEN=62) :: TEST_STR_2
    CHARACTER(LEN=62) :: TEST_STR_3
    CHARACTER(LEN=62) :: TEST_STR_4

    CHARACTER(LEN=62) :: TEST_OUT

    TEST_STR_1 = "| 01 02 03 04 05 06 07 08 09 10 99 44 99 44 99 44 99 44 99 44|"
    TEST_STR_2 = "| 01 02 03 04 05 06 07 08 09 99 44 99 44 99 44 99 44 99 44 99|"
    TEST_STR_3 = "| 01 02 03 04 05 06 07 08 09 10 99 44 99 44 99 44 99 44 99 44|"
    TEST_STR_4 = "| 01 05 09 44 44 02 06 99 99 99 03 07 44 44 44 04 08 99 99 99|"

    WRITE (UNIT=TEST_OUT, FMT=100) A1
    IF (TEST_STR_1 /= TEST_OUT) STOP "A1 FAILED"

    WRITE (UNIT=TEST_OUT, FMT=100) A2
    IF (TEST_STR_2 /= TEST_OUT) STOP "A2 FAILED"

    WRITE (UNIT=TEST_OUT, FMT=100) A3
    IF (TEST_STR_3 /= TEST_OUT) STOP "A3 FAILED"

    WRITE (UNIT=TEST_OUT, FMT=100) A4
    IF (TEST_STR_4 /= TEST_OUT) STOP "A4 FAILED"

    100 FORMAT ("|"20(I3.2)"|")
END PROGRAM P
