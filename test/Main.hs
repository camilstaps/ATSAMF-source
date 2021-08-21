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

-- |Describes a state of the paddle while sending a single character: either
-- one lever is squeezed ('SqueezeDash', 'SqueezeDot') or both ('SqueezeBoth').
data Input
  = SqueezeDash
  | SqueezeDot
  | SqueezeBoth
  deriving (Eq, Show)

instance Arbitrary Input where
  arbitrary = elements [SqueezeDash, SqueezeDot, SqueezeBoth]

-- |The input sequence for a single character is simply a '[Input]'.
type InputChar = [Input]

-- |The result of sending a character is a sequence of 'Dash'es and 'Dot's.
type ResultChar = [Result]

data Result
  = Dash
  | Dot
  deriving (Eq, Show)

-- |Checks that the 'ResultChar' for an 'InputChar' matches the expected value.
singleCharacter :: InputChar -> Property
singleCharacter input = monadicIO $ do
  output <- run (iambicKey input)
  stop $ output === expectedResult input :: PropertyM IO ()

-- |Gives the expected 'ResultChar' given an 'InputChar'.
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
expectedResult :: InputChar -> ResultChar
expectedResult = start
  where
    -- |The initial state, when no dashes and dots are queud.
    start [] = []
    start (SqueezeDot : rest) = playDot False rest
    start (input : rest) = playDash (input == SqueezeBoth) rest

    -- |Produce a 'Dot' and check the dash lever.
    playDot dashQueued input = Dot : playDot' dashQueued input

    playDot' dashQueued [] = [Dash | dashQueued]
    playDot' dashQueued (input : rest) = waitAfterDot
      (dashQueued || input == SqueezeDash || input == SqueezeBoth)
      rest

    -- |After playing a 'Dot', we are quiet for one dot time and check the dash
    -- lever.
    waitAfterDot dashQueued [] = [Dash | dashQueued]
    waitAfterDot dashQueued (input : rest)
      | dashQueued || input == SqueezeDash || input == SqueezeBoth =
        playDash (input == SqueezeDot || input == SqueezeBoth) rest
      | input == SqueezeDot =
        playDot False rest
      | otherwise =
        start rest

    -- |Poduce a 'Dash' and check the dot lever. We need a counter because
    -- dashes take thrice as long as dots.
    playDash dotQueued input = Dash : playDash' 2 dotQueued input

    playDash' _ dotQueued [] = [Dot | dotQueued]
    playDash' 0 dotQueued (input : rest) = waitAfterDash
      (dotQueued || input == SqueezeDot || input == SqueezeBoth)
      rest
    playDash' i dotQueued (input : rest) = playDash'
      (i - 1)
      (dotQueued || input == SqueezeDot || input == SqueezeBoth)
      rest

    -- |After playing a 'Dash', we are quiet for one dot time and check the dot
    -- lever.
    waitAfterDash dotQueued [] = [Dot | dotQueued]
    waitAfterDash dotQueued (input : rest)
      | dotQueued || input == SqueezeDot || input == SqueezeBoth =
        playDot (input == SqueezeDash || input == SqueezeBoth) rest
      | input == SqueezeDash =
        playDash False rest
      | otherwise =
        start rest

-- |To send an input sequence to C.
packInputChar :: InputChar -> String
packInputChar = map $ \case
  SqueezeDash -> '-'
  SqueezeDot -> '.'
  SqueezeBoth -> 'B'

-- |To receive a result sequence from C.
unpackResultChar :: String -> ResultChar
unpackResultChar = map $ \case
  '-' -> Dash
  '.' -> Dot

-- |Run a test case against the actual C implementation.
iambicKey :: InputChar -> IO ResultChar
iambicKey input =
  withCString (packInputChar input) $
  run_test >=> peekCString ^>> fmap unpackResultChar

foreign import ccall "test_key.h run_test" run_test :: CString -> IO CString
