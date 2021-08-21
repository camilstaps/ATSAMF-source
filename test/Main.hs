{-# LANGUAGE LambdaCase #-}
module Main where

import Control.Arrow
import Control.Monad
import Foreign.C.String
import System.Exit
import Test.QuickCheck hiding (Result)
import Test.QuickCheck.Monadic
import Test.QuickCheck.Random

{-|
  This file is used to test the iambic keying implementation in key.ino. It
  requires test_key.c as well to mock key.ino and run it in a regular
  environment (i.e. on your computer rather than on an arduino).

  In iambic keying the CW operator has a paddle with two levers: one to send
  the dot and one to send the dash. As long as the levers are not squeezed
  simultaneously, the behaviour is relatively simple: a dot or a dash is sent
  depending on which lever is squeezed. The iambic key mode defines what
  happens when the levers are squeezed simultaneously. When this happens, dots
  and dashes are sent alternatingly, starting with whichever lever was squeezed
  first and until one of the levers is released (after which we return to
  normal mode).
-}

main :: IO ()
main = do
  result <- quickCheckWithResult args singleCharacter
  unless (isSuccess result) exitFailure
  where
    args = stdArgs
      { replay = Just (mkQCGen 0, 0), -- Fix the random seed for reproducible counter-examples
        maxSuccess = 100000 -- Larger test suite by default
      }

-- |Describes a state of the paddle while sending: both levers can be squeezed
-- independent from each other.
data Input = Input
  { dashSqueezed :: Bool,
    dotSqueezed :: Bool
  }

-- |Dashes and dots are represented with '-' and '.'; when both are squeezed a
-- 'B' is used. Silence is represented as ' '.
instance Show Input where
  show input
    | dashSqueezed input = if dotSqueezed input then "B" else "-"
    | dotSqueezed input = "."
    | otherwise = " "

instance Arbitrary Input where arbitrary = Input <$> arbitrary <*> arbitrary

-- |The 'Arbitrary' instance takes care that the start and end of the input
-- sequence are non-silent (either dash or dot is squeezed, or both).
newtype InputSequence = InputSequence { getInputSequence :: NonEmptyList Input }

instance Show InputSequence where show = concatMap show . getNonEmpty . getInputSequence

instance Arbitrary InputSequence where arbitrary = InputSequence <$> arbitrary

-- |The result of sending one character is a sequence of '-'s and '.'s,
-- possibly separated by spaces.
type Result = String

-- |Checks that the 'Result' for an 'InputSequence' matches the expected value.
singleCharacter :: InputSequence -> Property
singleCharacter input = monadicIO $ do
  output <- run (iambicKey input)
  stop $ output === expectedResult input :: PropertyM IO ()

-- |Gives the expected 'Result' given an 'InputSequence'.
-- The principle here is that 'Input's alternate with state transitions in the
-- keyer.
--
-- For example, suppose the dot time is 100ms (so the dash time is 300ms). We
-- model things as if the switch from one input to the next happens at t=50ms,
-- t=150ms, etc. Schematically it looks like this:
--
-- Time:   0    50    100   150   200   250   300   350   400   450   500   550
-- Input:  | Dash|   Dash    |   Both    |   Both    |   (end of input)
-- Result: |   Dash    |   Dash    |   Dash    |  Silence   |   Dot    |  ...
--
-- The semantics can be described as a state machine, which is here represented
-- as a shallowly embedded DSL: the functions are states; the arguments are
-- memory. The states traverse the input sequence and produce the result
-- sequence.
expectedResult :: InputSequence -> Result
expectedResult = unwords . words . start . getNonEmpty . getInputSequence
  where
    -- |The initial state, when no dashes and dots are queud.
    -- When we are in this state for 7+ iterations, a new character is begun.
    start = start' 0

    start' 6 input = ' ' : start' 7 input
    start' i [] = []
    start' i (input : rest)
      | dashSqueezed input = playDash (dotSqueezed input) rest
      | dotSqueezed input = playDot False rest
      | otherwise = start' (i + 1) rest

    -- |Produce a '.' and check the dash lever.
    playDot dashQueued input = '.' : playDot' dashQueued input

    playDot' dashQueued [] = ['-' | dashQueued]
    playDot' dashQueued (input : rest) =
      waitAfterDot (dashQueued || dashSqueezed input) rest

    -- |After playing a '.', we are quiet for one dot time and check the dash
    -- lever.
    waitAfterDot dashQueued [] = ['-' | dashQueued]
    waitAfterDot dashQueued (input : rest)
      | dashQueued || dashSqueezed input = playDash (dotSqueezed input) rest
      | dotSqueezed input = playDot False rest
      | otherwise = start rest

    -- |Poduce a '-' and check the dot lever. We need a counter because dashes
    -- take thrice as long as dots.
    playDash dotQueued input = '-' : playDash' 2 dotQueued input

    playDash' _ dotQueued [] = ['.' | dotQueued]
    playDash' 0 dotQueued (input : rest) =
      waitAfterDash (dotQueued || dotSqueezed input) rest
    playDash' i dotQueued (input : rest) =
      playDash' (i - 1) (dotQueued || dotSqueezed input) rest

    -- |After playing a '-', we are quiet for one dot time and check the dot
    -- lever.
    waitAfterDash dotQueued [] = ['.' | dotQueued]
    waitAfterDash dotQueued (input : rest)
      | dotQueued || dotSqueezed input = playDot (dashSqueezed input) rest
      | dashSqueezed input = playDash False rest
      | otherwise = start rest

-- |To send an input sequence to C.
packInputSequence :: InputSequence -> String
packInputSequence = concatMap show . getNonEmpty . getInputSequence

-- |Run a test case against the actual C implementation.
iambicKey :: InputSequence -> IO Result
iambicKey input =
  withCString (packInputSequence input) $
  run_test >=> peekCString >>^ fmap (unwords . words)

foreign import ccall "test_key.h run_test" run_test :: CString -> IO CString
