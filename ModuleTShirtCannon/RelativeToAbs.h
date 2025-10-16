#ifndef relativetoabs_h
#define relativetoabs_h

/**
  Wraps the provided range with a clamping filter and managed offset producing this output
  max+offset --+-> max
               |
       value  -+> value-offset
               |
  min+offset --+-> min

  This ensures that the value delta provided by remote systems is 
  always preserved even if it doesn't know about local system constraints, 
  but the system itself is constrained to stable inputs on this end
*/
class RelativeToAbs{
  double min;
  double max;
  double offset;
  public:
  RelativeToAbs(double min_, double max_){
    min=min_;
    max=max_;
  }

  /** Declare value equality to reset the filter to known input/outputs */
  void set(double input, double output){
    offset=input-output;
  }

  /** Return input mapped to the desired range */
  double get(double input){
    //If input is out of range, nudge the range to comply with the input
    if(input <= min+offset ){
      offset = input-min;
    } else if( input >= max+offset){
      offset = input-max;
    }
    //Give back the normalized output
    return input - offset;
  }

};

#endif