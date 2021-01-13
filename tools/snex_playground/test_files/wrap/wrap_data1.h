/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "wrap/wrap_data1"
END_TEST_DATA
*/


/** A dummy object that expects a table. */
struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}
	
	void setExternalData(const ExternalData& d, int index)
	{
		//Console.print(125);
		d.referBlockTo(f, 0);
	}
	
	block f;
};



struct LookupTable
{
	span<float, 19> data = { 182.0f };
};

LookupTable lut;

ExternalData o = {lut};

wrap::data<X, data::embedded::table<LookupTable>> mainObject;

int main(int input)
{
	
	mainObject.setExternalData(o, 0);

	return mainObject.getWrappedObject().f.size();
}

